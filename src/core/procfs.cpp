// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/procfs.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include <pwd.h>
#include <unistd.h>

namespace sysograph {
namespace {

std::string readFileOr(const std::filesystem::path& path,
                       const std::string& fallback = {}) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return fallback;
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

std::vector<std::string> splitWhitespace(const std::string& s) {
  std::vector<std::string> out;
  std::istringstream iss(s);
  std::string tok;
  while (iss >> tok) out.push_back(tok);
  return out;
}

uint64_t toU64(const std::string& s) {
  try {
    return static_cast<uint64_t>(std::stoull(s));
  } catch (...) {
    return 0;
  }
}

// Skip virtual/ephemeral block devices when aggregating disk totals.
bool isVirtualDisk(const std::string& name) {
  static const char* prefixes[] = {"loop", "ram", "zram", "dm-", "sr",
                                   "md", "nbd"};
  for (const char* p : prefixes) {
    if (name.rfind(p, 0) == 0) return true;
  }
  return false;
}

}  // namespace

bool parse_stat_cpu_line(const std::string& line, bool* is_total,
                         CpuTimes* out) {
  if (line.rfind("cpu", 0) != 0) return false;
  // "cpu  ..." (aggregate) vs "cpu0 ..." (core).
  std::istringstream iss(line);
  std::string tag;
  iss >> tag;
  if (tag != "cpu" && tag.rfind("cpu", 0) != 0) return false;
  *is_total = (tag == "cpu");

  CpuTimes t;
  uint64_t v = 0;
  unsigned idx = 0;
  while (iss >> v) {
    switch (idx) {
      case 0: t.user = v; break;
      case 1: t.nice = v; break;
      case 2: t.system = v; break;
      case 3: t.idle = v; break;
      case 4: t.iowait = v; break;
      case 5: t.irq = v; break;
      case 6: t.softirq = v; break;
      case 7: t.steal = v; break;
      default: break;  // guest/guest_nice are folded into user elsewhere
    }
    ++idx;
  }
  if (idx < 4) return false;  // at least user/nice/system/idle required
  *out = t;
  return true;
}

bool parse_process_stat(const std::string& line, ProcessInfo* out) {
  // comm is parenthesised and may itself contain spaces or parentheses,
  // so split at the first '(' and the *last* ')'.
  const size_t open = line.find('(');
  if (open == std::string::npos) return false;
  const size_t close = line.rfind(')');
  if (close == std::string::npos || close < open) return false;

  ProcessInfo p;
  try {
    p.pid = std::stoi(line.substr(0, open));
  } catch (...) {
    return false;
  }
  p.name = line.substr(open + 1, close - open - 1);

  // Tokens after ") " map to stat fields 3..N.
  std::vector<std::string> v = splitWhitespace(line.substr(close + 1));
  auto field = [&](size_t stat_field) -> const std::string& {
    static const std::string empty;
    size_t i = stat_field - 3;
    return i < v.size() ? v[i] : empty;
  };

  if (v.empty()) return false;
  p.state = field(3).empty() ? '?' : field(3)[0];
  p.ppid = std::atoi(field(4).c_str());
  p.utime = toU64(field(14));
  p.stime = toU64(field(15));
  p.priority = std::atoi(field(18).c_str());
  p.nice = std::atoi(field(19).c_str());
  p.threads = std::max(1, std::atoi(field(20).c_str()));
  p.starttime_ticks = toU64(field(22));
  p.vsize_bytes = toU64(field(23));
  p.rss_bytes = toU64(field(24));  // in pages; scaled later
  p.last_cpu = std::atoi(field(39).c_str());
  *out = p;
  return true;
}

void parse_process_status(const std::string& content, ProcessInfo* out) {
  std::istringstream iss(content);
  std::string line;
  uint64_t rss_file = 0, rss_shmem = 0;
  while (std::getline(iss, line)) {
    const size_t colon = line.find(':');
    if (colon == std::string::npos) continue;
    const std::string key = line.substr(0, colon);
    const std::string value = line.substr(colon + 1);
    if (key == "Uid") {
      out->uid = static_cast<uint32_t>(toU64(splitWhitespace(value).empty()
                                                  ? "0"
                                                  : splitWhitespace(value)[0]));
    } else if (key == "Threads") {
      out->threads = std::max(1, static_cast<int>(toU64(value)));
    } else if (key == "RssFile") {
      rss_file = toU64(value) * 1024;
    } else if (key == "RssShmem") {
      rss_shmem = toU64(value) * 1024;
    }
  }
  out->shared_bytes = rss_file + rss_shmem;
}

bool parse_meminfo(const std::string& content, MemoryInfo* out) {
  std::istringstream iss(content);
  std::string line;
  bool any = false;
  while (std::getline(iss, line)) {
    const size_t colon = line.find(':');
    if (colon == std::string::npos) continue;
    const std::string key = line.substr(0, colon);
    // Values are in kB.
    const uint64_t kb = toU64(line.substr(colon + 1));
    const uint64_t bytes = kb * 1024;
    if (key == "MemTotal") { out->total = bytes; any = true; }
    else if (key == "MemFree") { out->free = bytes; }
    else if (key == "MemAvailable") { out->available = bytes; }
    else if (key == "Buffers") { out->buffers = bytes; }
    else if (key == "Cached") { out->cached = bytes; }
    else if (key == "SwapTotal") { out->swap_total = bytes; }
    else if (key == "SwapFree") { out->swap_free = bytes; }
  }
  return any;
}

bool parse_net_dev(const std::string& content, NetTotals* out) {
  std::istringstream iss(content);
  std::string line;
  bool any = false;
  while (std::getline(iss, line)) {
    const size_t colon = line.find(':');
    if (colon == std::string::npos) continue;
    const std::string iface = line.substr(0, colon);
    // Trim spaces of the interface name.
    size_t b = iface.find_first_not_of(" \t");
    size_t e = iface.find_last_not_of(" \t");
    if (b == std::string::npos) continue;
    const std::string name = iface.substr(b, e - b + 1);
    if (name == "lo") continue;  // loopback is not real traffic

    std::vector<std::string> t = splitWhitespace(line.substr(colon + 1));
    if (t.size() < 16) continue;
    out->rx_bytes += toU64(t[0]);
    out->tx_bytes += toU64(t[8]);
    any = true;
  }
  return any;
}

bool parse_diskstats(const std::string& content, DiskTotals* out) {
  std::istringstream iss(content);
  std::string line;
  bool any = false;
  while (std::getline(iss, line)) {
    std::vector<std::string> t = splitWhitespace(line);
    // major minor name reads_completed reads_merged sectors_read ms ...
    if (t.size() < 8) continue;
    if (isVirtualDisk(t[2])) continue;
    out->reads += toU64(t[3]);
    out->sectors_read += toU64(t[5]);
    // writes start at field 7 (index 7): writes_completed
    if (t.size() >= 11) {
      out->writes += toU64(t[7]);
      out->sectors_written += toU64(t[9]);
    }
    any = true;
  }
  return any;
}

std::vector<ServiceRow> parse_list_units(const std::string& output) {
  std::vector<ServiceRow> rows;
  std::istringstream iss(output);
  std::string line;
  while (std::getline(iss, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::vector<std::string> t = splitWhitespace(line);
    if (t.size() < 4) continue;
    // systemctl prints a header ("UNIT LOAD ACTIVE SUB DESCRIPTION") and
    // legend lines; both contain more structure or uppercase keywords.
    if (t[0] == "UNIT") continue;
    if (t[0].rfind("load", 0) == 0 || t[0] == "legend:") continue;
    if (!t[0].empty() && t[0].find(".service") == std::string::npos) {
      // Accept other unit types too, but the request filters to services.
      continue;
    }
    ServiceRow r;
    r.unit = t[0];
    r.load = t[1];
    r.active = t[2];
    r.sub = t[3];
    for (size_t i = 4; i < t.size(); ++i) {
      if (!r.description.empty()) r.description += ' ';
      r.description += t[i];
    }
    rows.push_back(std::move(r));
  }
  return rows;
}

Procfs::Procfs(std::filesystem::path root) : root_(std::move(root)) {}

bool Procfs::readCpuTimes(CpuTimes* total, std::vector<CpuTimes>* per_core) {
  const std::string content = readFileOr(root_ / "stat");
  std::istringstream iss(content);
  std::string line;
  bool got_total = false;
  per_core->clear();
  while (std::getline(iss, line)) {
    if (line.empty() || line[0] != 'c') continue;  // only cpu lines
    bool is_total = false;
    CpuTimes t;
    if (!parse_stat_cpu_line(line, &is_total, &t)) continue;
    if (is_total) {
      *total = t;
      got_total = true;
    } else {
      per_core->push_back(t);
    }
  }
  return got_total;
}

bool Procfs::readMemory(MemoryInfo* out) {
  return parse_meminfo(readFileOr(root_ / "meminfo"), out);
}

bool Procfs::readNetwork(NetTotals* out) {
  return parse_net_dev(readFileOr(root_ / "net" / "dev"), out);
}

bool Procfs::readDisks(DiskTotals* out) {
  return parse_diskstats(readFileOr(root_ / "diskstats"), out);
}

bool Procfs::readUptime(double* seconds) {
  std::istringstream iss(readFileOr(root_ / "uptime"));
  double up = 0.0;
  if (!(iss >> up) || up <= 0.0) return false;
  *seconds = up;
  return true;
}

bool Procfs::readProcsCounts(unsigned* running, unsigned* blocked) {
  const std::string content = readFileOr(root_ / "stat");
  std::istringstream iss(content);
  std::string line;
  bool any = false;
  while (std::getline(iss, line)) {
    if (line.rfind("procs_running", 0) == 0) {
      *running = static_cast<unsigned>(toU64(line.substr(14)));
      any = true;
    } else if (line.rfind("procs_blocked", 0) == 0) {
      *blocked = static_cast<unsigned>(toU64(line.substr(14)));
      any = true;
    }
  }
  return any;
}

std::vector<ProcessInfo> Procfs::listProcesses() {
  std::vector<ProcessInfo> out;
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator(root_, ec)) {
    const std::string name = entry.path().filename().string();
    if (name.empty() || !std::isdigit(static_cast<unsigned char>(name[0]))) {
      continue;
    }
    int pid = std::atoi(name.c_str());
    if (pid <= 0) continue;

    const std::string stat = readFileOr(entry.path() / "stat");
    ProcessInfo p;
    if (!parse_process_stat(stat, &p)) continue;
    p.pid = pid;

    parse_process_status(readFileOr(entry.path() / "status"), &p);
    p.rss_bytes *= static_cast<uint64_t>(pageSize());
    p.user = userName(p.uid);

    // cmdline is NUL-separated; kernel threads have an empty cmdline.
    std::string cmdline = readFileOr(entry.path() / "cmdline");
    if (!cmdline.empty()) {
      for (char& c : cmdline) {
        if (c == '\0') c = ' ';
      }
      while (!cmdline.empty() && cmdline.back() == ' ') cmdline.pop_back();
    }
    p.cmdline = std::move(cmdline);
    out.push_back(std::move(p));
  }
  std::sort(out.begin(), out.end(),
            [](const ProcessInfo& a, const ProcessInfo& b) { return a.pid < b.pid; });
  return out;
}

std::string Procfs::userName(uint32_t uid) {
  auto it = user_cache_.find(uid);
  if (it != user_cache_.end()) return it->second;

  passwd pwd;
  passwd* result = nullptr;
  std::vector<char> buf(4096);
  std::string name;
  if (getpwuid_r(static_cast<uid_t>(uid), &pwd, buf.data(), buf.size(),
                 &result) == 0 &&
      result != nullptr) {
    name = result->pw_name;
  }
  if (name.empty()) name = std::to_string(uid);
  user_cache_[uid] = name;
  return name;
}

std::string Procfs::readLinkField(const std::filesystem::path& proc,
                                  const char* field) {
  std::error_code ec;
  std::filesystem::path target =
      std::filesystem::read_symlink(proc / field, ec);
  if (ec) return {};
  return target.string();
}

int Procfs::countFileDescriptors(const std::filesystem::path& proc) {
  int count = 0;
  std::error_code ec;
  std::filesystem::path fd = proc / "fd";
  if (!std::filesystem::exists(fd, ec)) return -1;
  for (std::filesystem::directory_iterator it(fd, ec), end; !ec && it != end;
       it.increment(ec)) {
    ++count;
  }
  if (ec) return count > 0 ? count : -1;
  return count;
}

long Procfs::clockTicksPerSecond() const { return sysconf(_SC_CLK_TCK); }

long Procfs::pageSize() const { return sysconf(_SC_PAGESIZE); }

}  // namespace sysograph

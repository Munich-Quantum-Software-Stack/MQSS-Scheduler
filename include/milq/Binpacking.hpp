#ifndef MILQ_BINPACKING_HPP
#define MILQ_BINPACKING_HPP

#include <Schedule.hpp>
#include <string>
#include <vector>

namespace milq
{

// TODO Could be done more generic -> template?
// Binpacking is the baseline
void do_bin_packing(const std::vector<JobProxy> &proxies,
                    std::vector<Machine *> machines,
                    bool use_preferences = true);
} // namespace milq

#endif // MILQ_BINPACKING_HPP

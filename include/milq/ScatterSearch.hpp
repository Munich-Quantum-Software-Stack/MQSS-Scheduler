#ifndef MILQ_SCATTER_SEARCH_HPP
#define MILQ_SCATTER_SEARCH_HPP

#include <Binpacking.hpp>
#include <Schedule.hpp>
#include <iterator>
#include <qdmi.h>
#include <random>
#include <vector>

namespace milq
{
// Scatter search could be implemented as meta heuristic
// It could improve any existing schedule
Schedule scatter_search(std::vector<JobProxy> &proxies,
                        std::vector<QDMI_Device> *devices,
                        int n_iterations = 1000, int n_solutions = 10);

} // namespace milq

#endif // MILQ_SCATTER_SEARCH_HPP

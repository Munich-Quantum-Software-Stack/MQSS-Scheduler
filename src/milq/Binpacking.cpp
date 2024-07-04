#include <Binpacking.hpp>

namespace milq
{

struct Bin
{
    // Hepler struct for bin packing
    // Conceptually similar to a Bucket
    int index;
    int capacity;
    int qpu;
    bool full;
    std::string device_name;
    std::vector<JobProxy> jobs;
};

size_t find_fitting_bin(const JobProxy &proxy, const std::vector<Bin> *bins,
                        bool use_preferences = false)
{
    if (use_preferences)
    {
        for (size_t idx = 0; idx < bins->size(); idx++)
        {
            const auto bin = bins->at(idx);
            if (bin.capacity >= proxy.num_qubits &&
                proxy.preffered_qpu == bin.device_name)
            {
                return idx;
            }
        }
    }
    for (size_t idx = 0; idx < bins->size(); idx++)
    {
        const auto bin = bins->at(idx);
        if (bin.capacity >= proxy.num_qubits)
        {
            return idx;
        }
    }
    return -1;
}

void do_bin_packing(const std::vector<JobProxy> &proxies,
                    std::vector<Machine> *machines, bool use_preferences)
{
    // First fit decreasing bin packing
    std::vector<Bin> open_bins;
    for (size_t idx = 0; idx < machines->size(); idx++)
    {
        open_bins.push_back(
            {0,
             machines->at(idx).capacity,
             static_cast<int>(idx),
             false,
             strrchr(machines->at(idx).device_ref->library.libname, '/'),
             {}});
    }
    std::vector<Bin> closed_bins;
    int index = 1;

    for (const auto &proxy : proxies)
    {
        // Find the index of a fitting bin
        auto bin_idx = find_fitting_bin(proxy, &open_bins, use_preferences);

        if (bin_idx == -1)
        {
            // No fittting bin found
            // Opening new bins
            std::vector<Bin> new_bins;
            for (size_t idx = 0; idx < machines->size(); idx++)
            {
                const auto machine = machines->at(idx);
                new_bins.push_back(
                    {index,
                     machine.capacity,
                     static_cast<int>(idx),
                     false,
                     strrchr(machine.device_ref->library.libname, '/'),
                     {}});
            }
            index++;
            bin_idx = find_fitting_bin(proxy, &new_bins, use_preferences);
            // if (bin_idx == -1)  -> we don't have a large enough device
            // add new bins to open bins
            bin_idx += open_bins.size();
            open_bins.insert(open_bins.end(), new_bins.begin(), new_bins.end());
        }
        // Add job to selected bin
        auto &selected_bin = open_bins[bin_idx];
        selected_bin.jobs.push_back(proxy);
        selected_bin.capacity -= proxy.num_qubits;
        // Close bin if full
        if (selected_bin.capacity == 0)
        {
            selected_bin.full = true;
            closed_bins.push_back(selected_bin);
            open_bins.erase(open_bins.begin() + bin_idx);
        }
    }
    // Close all open bins
    for (auto &obin : open_bins)
    {
        if (!obin.jobs.empty())
        {
            closed_bins.push_back(obin);
        }
    }
    // Turn bins into buckets
    for (auto &bin : closed_bins)
    {
        std::list<JobProxy *> jobs;
        for (auto &job : bin.jobs)
        {
            jobs.push_back(&job);
        }
        Bucket bucket = {.job_proxies = jobs};
        machines->at(bin.qpu).buckets.push_back(&bucket);
    }
}

} // namespace milq

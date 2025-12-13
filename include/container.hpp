// Container class for SimCpp20 - analogous to SimPy's Container
// Manages continuous resources like RAM

#pragma once

#include <fschuetz04/simcpp20.hpp>
#include <queue>
#include <stdexcept>

namespace simcpp20 {

/**
 * A container holding a continuous resource (like RAM).
 * Analogous to SimPy's Container.
 *
 * @tparam Time Type used for simulation time.
 */
template <typename Time = double>
class container {
public:
    /**
     * Constructor.
     *
     * @param sim Reference to the simulation.
     * @param capacity Maximum capacity of the container.
     * @param init Initial level of the container.
     */
    explicit container(simulation<Time>& sim, uint64_t capacity, uint64_t init = 0)
        : sim_{sim}, capacity_{capacity}, level_{init} {
        if (init > capacity) {
            throw std::invalid_argument("Initial level exceeds capacity");
        }
    }

    /**
     * Get an amount from the container.
     *
     * @param amount Amount to get.
     * @return An event that will be triggered once enough is available.
     */
    event<Time> get(uint64_t amount) {
        if (amount > capacity_) {
            throw std::invalid_argument("Requested amount exceeds container capacity");
        }

        auto ev = sim_.event();

        if (level_ >= amount) {
            // Enough available, grant immediately
            level_ -= amount;
            ev.trigger();
            process_put_queue(); // May allow pending puts
        } else {
            // Not enough, queue the request
            get_queue_.push(GetRequest{amount, std::move(ev)});
        }

        return ev;
    }

    /**
     * Put an amount into the container.
     *
     * @param amount Amount to put.
     * @return An event that will be triggered once there is capacity.
     */
    event<Time> put(uint64_t amount) {
        if (amount > capacity_) {
            throw std::invalid_argument("Put amount exceeds container capacity");
        }

        auto ev = sim_.event();

        if (level_ + amount <= capacity_) {
            // Room available, put immediately
            level_ += amount;
            ev.trigger();
            process_get_queue(); // May allow pending gets
        } else {
            // Not enough room, queue the request
            put_queue_.push(PutRequest{amount, std::move(ev)});
        }

        return ev;
    }

    /// @return Current level in the container.
    uint64_t level() const { return level_; }

    /// @return Capacity of the container.
    uint64_t capacity() const { return capacity_; }

private:
    /// Reference to the simulation.
    simulation<Time>& sim_;

    /// Capacity of the container.
    uint64_t capacity_;

    /// Current level in the container.
    uint64_t level_;

    /// Pending get requests.
    struct GetRequest {
        uint64_t amount;
        event<Time> ev;
    };
    std::queue<GetRequest> get_queue_;

    /// Pending put requests.
    struct PutRequest {
        uint64_t amount;
        event<Time> ev;
    };
    std::queue<PutRequest> put_queue_;

    /// Process pending get requests.
    void process_get_queue() {
        while (!get_queue_.empty()) {
            auto req = std::move(get_queue_.front());
            if (req.ev.aborted()) {
                get_queue_.pop();
                continue;
            }

            if (level_ >= req.amount) {
                level_ -= req.amount;
                req.ev.trigger();
                get_queue_.pop();
            } else {
                break; // Not enough, stop processing
            }
        }
    }

    /// Process pending put requests.
    void process_put_queue() {
        while (!put_queue_.empty()) {
            auto req = std::move(put_queue_.front());
            if (req.ev.aborted()) {
                put_queue_.pop();
                continue;
            }

            if (level_ + req.amount <= capacity_) {
                level_ += req.amount;
                req.ev.trigger();
                put_queue_.pop();
            } else {
                break; // Not enough room, stop processing
            }
        }
    }
};

} // namespace simcpp20

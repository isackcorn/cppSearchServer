#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "log_duration.h"

using namespace std::string_literals;

template<typename Key, typename Value>
class ConcurrentMap {
private:
    struct Bucket;

public:

    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value &ref_to_value;

        Access(const Key &key, Bucket &bucket)
                : guard(bucket.mut_),
                  ref_to_value(bucket.map_[key]) {}
    };

    explicit ConcurrentMap(size_t bucket_count)
            : buckets_(bucket_count) {}

    Access operator[](const Key &key) {
        auto &bucket = buckets_[key % buckets_.size()];
        return {key, bucket};
    }

    void Erase(const Key &key) {
        auto temp = key % buckets_.size();
        std::lock_guard guard(buckets_[temp].mut_);
        buckets_[temp].map_.erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto &[mutex, map]: buckets_) {
            std::lock_guard grd(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }


private:
    struct Bucket {
        std::mutex mut_;
        std::map<Key, Value> map_;
    };

    std::vector<Bucket> buckets_;
};

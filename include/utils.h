#pragma once

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <vector>

/**
 * @namespace utils
 * @brief Utility functions for type conversion, randomness, container manipulation, sorting, and geographic calculations.
 * 
 * @details Categories:
 * - Type Conversion: toString, toPercentageString, vecToString, setToString.
 * - Randomness: getEngine, randomNumber, shuffledRange, randomPoisson, generatePoissonSchedule.
 * - Containers: sliceBySchedule, appendVector, mergeVectors, mergeSets.
 * - Sorting: sortEntities.
 * - Geographic: haversineDistance, calculateDistance.
 */
namespace utils {

    #pragma region Type_Conversion_Utilities
    //=========================================================================
    // Type Conversion Utilities
    //=========================================================================

    /**
     * @brief Converts a value of any type to its string representation.
     * @details Uses `std::ostringstream` for conversion, applying fixed-point notation with specified precision for floating-point types.
     * @tparam T The data type of the value to be converted.
     * @param value The value to convert.
     * @param precision The number of decimal places for floating-point types.
     * @return A string representation of the input value.
     */
    template <typename T>
    inline std::string toString(const T& value, int precision = 6) {
        std::ostringstream out;
        if constexpr (std::is_floating_point_v<T>) {
            out << std::fixed << std::setprecision(precision);
        }
        out << value;
        return out.str();
    }

    /**
     * @brief Calculates a percentage and converts it to a formatted string.
     * @details Computes `(numerator / denominator) * 100.0` and returns it as a string. Safely handles division by zero.
     * @param numerator The value representing the part of the total.
     * @param denominator The value representing the total.
     * @param precision The number of decimal places for the resulting percentage string.
     * @return A string representation of the calculated percentage.
     */
    inline std::string toPercentageString(double numerator, double denominator, int precision = 4) {
        if (denominator == 0) {
            return toString(0.0, precision);
        }
        double percentage = (numerator / denominator) * 100.0;
        return toString(percentage, precision);
    }

    /**
     * @brief Converts a vector of any primitive type to a JSON-like string representation.
     * @tparam T The type of elements in the vector.
     * @param vec The vector to convert.
     * @return A string formatted as an array (e.g., "[e1,e2,...]").
     */
    template <typename T>
    inline std::string vecToString(const std::vector<T>& vec) {
        if (vec.empty()) return "[]";
        std::string str = "[";
        for (size_t i = 0; i < vec.size(); ++i) {
            // std::to_string handles all standard numeric types
            str += std::to_string(vec[i]);
            
            if (i < vec.size() - 1) str += ",";
        }
        str += "]";
        return str;
    }

    /**
     * @brief Converts a set of any primitive type to a JSON-like string representation.
     * @tparam T The type of elements in the set.
     * @param s The set to convert.
     * @return A string formatted as an array (e.g., "[e1,e2,...]").
     */
    template <typename T>
    inline std::string setToString(const std::set<T>& s) {
        if (s.empty()) return "[]";
        std::string str = "[";
        auto it = s.begin();
        while (it != s.end()) {
            str += std::to_string(*it);
            ++it;
            if (it != s.end()) str += ",";
        }
        str += "]";
        return str;
    }
    #pragma endregion

    #pragma region Randomness_Utilities
    //=========================================================================
    // Randomness Utilities
    //=========================================================================

    /**
     * @brief Provides a singleton instance of a high-quality random number engine.
     * @details Returns a reference to a static `std::mt19937` engine seeded with `std::random_device`.
     * @return A reference to the static `std::mt19937` random engine.
     */
    inline std::mt19937& getEngine() {
        static std::random_device rd;
        static std::mt19937 engine(rd());
        return engine;
    }

    /**
     * @brief Generates a random number within a specified inclusive interval.
     * @tparam T The numeric type (must be arithmetic).
     * @param min The lower bound of the interval (inclusive).
     * @param max The upper bound of the interval (inclusive).
     * @return A random number within the specified range.
     * @throws std::invalid_argument If min is greater than max.
     */
    template<typename T>
    inline T randomNumber(T min, T max) {
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type.");
        if (min > max) {
            throw std::invalid_argument("Error in randomNumber: min cannot be greater than max.");
        }
        if constexpr (std::is_integral_v<T>) {
            std::uniform_int_distribution<T> distribution(min, max);
            return distribution(getEngine());
        } else {
            std::uniform_real_distribution<T> distribution(min, max);
            return distribution(getEngine());
        }
    }

    /**
     * @brief Creates a vector of unique integers in random order within an inclusive range.
     * @tparam T The integral type for the range.
     * @param min The lower bound of the range (inclusive).
     * @param max The upper bound of the range (inclusive).
     * @return A randomly shuffled vector containing all integers from min to max.
     * @throws std::invalid_argument If min is greater than max.
     */
    template<typename T>
    inline std::vector<T> shuffledRange(T min, T max) {
        static_assert(std::is_integral_v<T>, "shuffledRange requires an integral type.");
        if (min > max) {
            throw std::invalid_argument("Error in shuffledRange: min cannot be greater than max.");
        }
        std::vector<T> numbers(max - min + 1);
        std::iota(numbers.begin(), numbers.end(), min);
        std::shuffle(numbers.begin(), numbers.end(), getEngine());
        return numbers;
    }

    /**
     * @brief Generates a random number following a Poisson distribution.
     * @param lambda The expected mean number of arrivals per time unit.
     * @return An integer representing the number of arrivals.
     */
    inline int randomPoisson(double lambda) {
        std::poisson_distribution<int> distribution(lambda);
        return distribution(getEngine());
    }

    /**
     * @brief Generates a deterministic schedule based on a shifted Poisson distribution matching a target sum.
     * @param start The minimum value for each generated element.
     * @param target_total The exact required sum of all elements in the returned vector.
     * @param lambda The mean value for the Poisson distribution.
     * @return A sequence of values summing exactly to target_total.
     */
    inline std::vector<int> generatePoissonSchedule(int start, int target_total, double lambda) {
        std::vector<int> schedule;
        int current_sum = 0;

        // Proteção contra parâmetros inválidos
        if (target_total <= 0 || start <= 0) {
            return schedule; 
        }

        while (current_sum < target_total) {
            // Desloca a distribuição: o valor mínimo sempre será 'start'
            int generated_value = start + randomPoisson(lambda);

            // Ajusta o último valor para bater exatamente com o target total (truncamento)
            if (current_sum + generated_value > target_total) {
                generated_value = target_total - current_sum;
            }

            schedule.push_back(generated_value);
            current_sum += generated_value;
        }

        return schedule;
    }

    /**
     * @brief Generates a sequence of a specific length using a shifted Poisson distribution.
     * @param start The minimum value for each generated element.
     * @param num_steps The exact number of elements in the generated vector.
     * @param lambda The mean value for the Poisson distribution.
     * @return A sequence of exactly num_steps values.
     */
    inline std::vector<int> generatePoissonSchedule(int start, size_t num_steps, double lambda) {
        std::vector<int> schedule;

        if (num_steps == 0 || start <= 0) {
            return schedule;
        }

        schedule.reserve(num_steps);
        for (size_t i = 0; i < num_steps; ++i) {
            schedule.push_back(start + randomPoisson(lambda));
        }

        return schedule;
    }
    #pragma endregion

    #pragma region Container_Utilities
    //=========================================================================
    // Container Utilities
    //=========================================================================

    /**
     * @brief Slices a flat vector into a matrix (vector of vectors) based on a schedule.
     * @details Position [0] represents an empty step. Subsequent vectors contain sequential slices of the source.
     * @tparam T The type of elements in the source vector.
     * @param source The flat vector containing all elements to be distributed.
     * @param schedule A vector of integers where each value represents the number of elements for that step.
     * @return A matrix representing the data distributed over time.
     */
    template <typename T>
    inline std::vector<std::vector<T>> sliceBySchedule(const std::vector<T>& source, const std::vector<int>& schedule) {
        std::vector<std::vector<T>> matrix;
        matrix.reserve(schedule.size() + 1);

        // t = 0: Nothing happens (Empty vector)
        matrix.push_back(source);

        auto it = source.begin();
        for (int count : schedule) {
            std::vector<T> batch;
            batch.reserve(count);

            // Copy 'count' elements or until source runs out
            for (int i = 0; i < count && it != source.end(); ++i) {
                batch.push_back(*it);
                ++it;
            }
            
            matrix.push_back(std::move(batch));
        }

        return matrix;
    }

    /**
     * @brief Generates an increasing sequence and slices it based on a schedule.
     * @tparam T The numeric type for the sequence (e.g., int, long).
     * @param start The starting value of the sequence.
     * @param end The ending value of the sequence (inclusive).
     * @param schedule A vector of integers representing the sizes of each slice.
     * @return A matrix representing the sliced sequence over time.
     */
    template <typename T>
    inline std::vector<std::vector<T>> sliceBySchedule(T start, T end, const std::vector<int>& schedule) {
        std::vector<T> generated_source;

        if (start <= end) {
            size_t tamanho = static_cast<size_t>(end - start + 1);
            generated_source.resize(tamanho);
            
            std::iota(generated_source.begin(), generated_source.end(), start);
        }

        return sliceBySchedule(generated_source, schedule);
    }
    /**
     * @brief Appends the contents of the source vector to the destination vector.
     * @tparam T The type of elements in the vectors.
     * @param dest The destination vector to be modified.
     * @param src The source vector to be appended.
     */
    template <typename T>
    inline void appendVector(std::vector<T>& dest, const std::vector<T>& src) {
        dest.insert(dest.end(), src.begin(), src.end());
    }

    /**
     * @brief Merges two vectors into a new single vector.
     * @tparam T The type of elements in the vectors.
     * @param v1 The first vector.
     * @param v2 The second vector.
     * @return A new vector containing elements from v1 followed by v2.
     */
    template <typename T>
    inline std::vector<T> mergeVectors(const std::vector<T>& v1, const std::vector<T>& v2) {
        std::vector<T> result;
        result.reserve(v1.size() + v2.size());
        result.insert(result.end(), v1.begin(), v1.end());
        result.insert(result.end(), v2.begin(), v2.end());
        return result;
    }

    /**
     * @brief Merges two sets into a new single set.
     * @tparam T The type of elements in the sets.
     * @param s1 The first set.
     * @param s2 The second set.
     * @return A new set containing unique elements from both s1 and s2.
     */
    template <typename T>
    inline std::set<T> mergeSets(const std::set<T>& s1, const std::set<T>& s2) {
        std::set<T> result = s1;
        result.insert(s2.begin(), s2.end());
        return result;
    }

    #pragma endregion

    #pragma region Sorting_Utilities
    //=========================================================================
    // Sorting Utilities
    //=========================================================================
    
    /**
     * @brief Internal details for generic implementations.
     */
    namespace details {

        /**
         * @brief Internal sorting implementation logic that maps a member attribute.
         * @tparam Ascending Sort direction flag.
         * @tparam T Entity struct/class type.
         * @tparam AttributeType Data type of the sorted attribute.
         * @tparam AttributePtr Pointer-to-member specifying the comparison attribute.
         * @param entities Constant reference to the data vector.
         * @param idxs Vector of indices to be sorted.
         */
        template <bool Ascending, typename T, typename AttributeType, AttributeType T::* AttributePtr>
        void sort_indices_impl(const std::vector<T>& entities, std::vector<int>& idxs) {
            std::sort(idxs.begin(), idxs.end(), [&](int i, int j) {
                const auto& attr_i = entities.at(i).*AttributePtr;
                const auto& attr_j = entities.at(j).*AttributePtr;

                if (attr_i != attr_j) {
                    if constexpr (Ascending) {
                        return attr_i < attr_j; // Ascending order
                    } else {
                        return attr_i > attr_j; // Descending order
                    }
                }
                return i < j; // Tie-breaker
            });
        }
    }

    /**
     * @brief Sorts the indices of all entities (from 1 to N-1) by a specific struct/class attribute.
     * @details Original vector is not modified. Assumes 1-based indexing for internal simulation logic. 
     * @tparam Ascending If true, sorts in ascending order; otherwise, descending.
     * @tparam T The type of the entity.
     * @tparam AttributeType The data type of the member attribute.
     * @tparam AttributePtr Pointer-to-member specifying the attribute for comparison.
     * @param entities The constant vector of entities.
     * @return A new vector containing the sorted indices.
     */
    template <bool Ascending, typename T, typename AttributeType, AttributeType T::* AttributePtr>
    inline std::vector<int> sortEntities(const std::vector<T>& entities) {
        if (entities.size() <= 1) return {};
        std::vector<int> idxs(entities.size() - 1);
        std::iota(idxs.begin(), idxs.end(), 1); // Assumes 1-based indexing
        details::sort_indices_impl<Ascending, T, AttributeType, AttributePtr>(entities, idxs);
        return idxs;
    }

    /**
     * @brief Sorts a subset of entity indices by a specific attribute.
     * @tparam Ascending If true, sorts in ascending order; otherwise, descending.
     * @tparam T The type of the entity.
     * @tparam AttributeType The data type of the member attribute.
     * @tparam AttributePtr Pointer-to-member specifying the attribute for comparison.
     * @param entities The constant vector of entities.
     * @param indices_to_sort A vector of integers representing the subset of indices to sort.
     * @return A new vector containing the sorted subset indices.
     */
    template <bool Ascending, typename T, typename AttributeType, AttributeType T::* AttributePtr>
    inline std::vector<int> sortEntities(const std::vector<T>& entities, const std::vector<int>& indices_to_sort) {
        if (entities.empty() || indices_to_sort.empty()) return {};
        std::vector<int> idxs = indices_to_sort; // Create a copy to sort
        details::sort_indices_impl<Ascending, T, AttributeType, AttributePtr>(entities, idxs);
        return idxs;
    }
    #pragma endregion

    #pragma region Geographic_Utilities
    //=========================================================================
    // Geographic Utilities
    //=========================================================================

    /**
     * @brief The mean radius of the Earth in kilometers.
     */
    constexpr long double EARTH_RADIUS_KM = 6371.0088L;
    
    /**
     * @brief High-precision definition of Pi.
     */
    constexpr long double PI_L = 3.141592653589793238462643383279502884L;

    /**
     * @brief Calculates the distance between two points using the Haversine formula in radians.
     * @param lat1_rad Latitude of point 1 in radians.
     * @param lon1_rad Longitude of point 1 in radians.
     * @param lat2_rad Latitude of point 2 in radians.
     * @param lon2_rad Longitude of point 2 in radians.
     * @return The great-circle distance in kilometers.
     */
    inline double haversineDistance(long double lat1_rad, long double lon1_rad, long double lat2_rad, long double lon2_rad) {
        const long double dlat = lat2_rad - lat1_rad;
        const long double dlon = lon2_rad - lon1_rad;
        
        const long double a = pow(sin(dlat / 2.0L), 2) + cos(lat1_rad) * cos(lat2_rad) * pow(sin(dlon / 2.0L), 2);
        const long double c = 2.0L * atan2(sqrt(a), sqrt(1.0L - a));
        
        return static_cast<double>(EARTH_RADIUS_KM * c);
    }

    /**
     * @brief Calculates the distance between two geographic coordinates given in degrees.
     * @details Uses a fast equirectangular approximation for distances under 1.5km, and falls back to Haversine for longer distances.
     * @param lat1_deg Latitude of point 1 in degrees.
     * @param lon1_deg Longitude of point 1 in degrees.
     * @param lat2_deg Latitude of point 2 in degrees.
     * @param lon2_deg Longitude of point 2 in degrees.
     * @return The distance between the two points in kilometers.
     */
    inline double calculateDistance(double lat1_deg, double lon1_deg, double lat2_deg, double lon2_deg) {
        const long double lat1_rad = lat1_deg * (PI_L / 180.0L);
        const long double lon1_rad = lon1_deg * (PI_L / 180.0L);
        const long double lat2_rad = lat2_deg * (PI_L / 180.0L);
        const long double lon2_rad = lon2_deg * (PI_L / 180.0L);

        const double threshold_km = 1.5;
        const long double x = (lon2_rad - lon1_rad) * cos((lat1_rad + lat2_rad) / 2.0L);
        const long double y = (lat2_rad - lat1_rad);
        const double planar_distance = static_cast<double>(sqrt(x * x + y * y) * EARTH_RADIUS_KM);
        
        if (planar_distance < threshold_km) {
            return planar_distance;
        }
        
        return haversineDistance(lat1_rad, lon1_rad, lat2_rad, lon2_rad);
    }
    #pragma endregion
}
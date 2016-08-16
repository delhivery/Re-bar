//
// Created by amitprakash on 8/1/16.
//

#ifndef FLETCHER_STRUCTURES_HXX
#define FLETCHER_STRUCTURES_HXX

#include <limits>
#include <experimental/string_view>
#include <utility>

#include "constants.hxx"

/**
 * @brief A structure representing cost in the graph
 */
struct Cost {
    double expense;
    long duration;

    Cost(bool infinite=true);

    Cost(double expense_, long duration_) : expense(expense_), duration(duration_) {}

    long time() const {return duration;}

    double cost() const { return expense;}

    bool is_infinite() const;

    Cost& operator = (Cost operand);
};

/**
 * @brief Structure representing properties of an edge in the graph
 */
template <typename Features, typename Cost>
struct EdgeProperty {
    /**
     * @brief Attribute specifying if the edge is a transient/custody edge
     */
    bool percon = false;

    /**
     * @brief Attribute specifying human readable representation of edge
     */
    std::string code;

    /**
     * @brief Actual departure time at source
     */
    long _dep;

    /**
     * @brief Actual duration to transit to destintion
     */
    long _dur;

    /**
     * @brief Physical cost of traversing the edge
     */
    double cost;

    /**
     * @brief Time to aggregate at source
     */
    long _tap;

    /**
     * @brief Time to outbound at source
     */
    long _top;

    /**
     * @brief Departure cutoff for edge
     */
    long dep;

    /**
     * @brief Time to inbound at destination
     */
    long _tip;

    /**
     * @brief Features supported by edge
     */
    Features features;

    /**
     * @brief Default construct an empty edge property
     */
    EdgeProperty() = default;

    /**
     * @brief Construct edge property for a transient/custody edge
     * @param[in] tap Aggregation processing time at source
     * @param[in] top Outbound processing time at source
     * @param[in] tip Inbound processing time at source
     * @param[in] cost Physical cost to traverse the edge
     * @param[in] code Human readable representation for edge
     * @param[in] features Map of features supported by edge
     */
    EdgeProperty(
            const long tap_,
            const long top_,
            const long tip_,
            const double cst_,
            std::experimental::string_view con_,
            const Features& features_) :
        code(con_.to_string()),
        cost(cst_),
        _tap(tap_),
        _top(top_),
        _tip(tip_),
        features(features_) { percon = true;}

    /**
     * @brief Construct an edg property for non-transient edge
     * @param[in] departure Departure time at source
     * @param[in] duration Duration for traversal from source to destination
     * @param[in] tap Aggregation processing time at source
     * @param[in] top Outbound processing time at source
     * @param[in] tip Inbound processing time at destination
     * @param[in] cost Physical cost to traverse the edge
     * @param[in] code Human readable representation for edge
     * @param[in] features Map of features supported by edge
     */
    EdgeProperty(
            const long dep_,
            const long dur_,
            const long tap_,
            const long top_,
            const long tip_,
            const double cst_,
            std::experimental::string_view con_,
            const Features& features_) :
        code(con_.to_string()),
        _dep(dep_),
        _dur(dur_),
        cost(cst_),
        _tap(tap_),
        _top(top_),
        _tip(tip_),
        features(features_) {}

    /**
     * @brief Calculate the wait time at this edge before traversal
     * @param arrival Time of arrival at edge source
     * @return Time duration to wait before traversing this edge
     */
    long wait_time(
            const long arrival) const {
        if(percon) {
            return 0;
        }

        auto arrival_durinal = arrival % TIME_DURINAL;
        return (arrival_durinal > dep) ? (TIME_DURINAL - arrival_durinal + dep) : (dep - arrival_durinal);
    }

    /**
     * @brief Return the Cost to traverse this edge
     * @param arrival Cost to arrive at source
     * @param limits Maximum allowed cost to traverse to destination
     * @return Cost on traversing this edge
     */
    Cost weight(const Cost& arrival, const Cost& limits) const {
        Cost expense;

        if (percon) {
            expense = Cost{_tap + _top + _tip, cost};
        }
        else {
            expense = Cost{wait_time(arrival.time()) + _tap + _top + _dur + _tip, cost};
        }

        if (expense > limits) {
            expense = Cost(true);
        }
        return expense;
    }
};

struct Segment {
    std::string src;
    std::string dst;
    std::string con;

    long arrival_source;
    long max_arrival_source;
    long departure_source;
    double cost_arrival;

    Segment() = default;

    Segment(
            std::experimental::string_view src_,
            std::experimental::string_view dst_,
            std::experimental::string_view con_,
            const long arr_src_,
            const long max_arr_src_,
            const long dep_src_,
            const double cst_arr_) : src(src_.to_string()),
                                     dst(dst_.to_string()),
                                     con(con_.to_string()),
                                     arrival_source(arr_src_),
                                     max_arrival_source(max_arr_src_),
                                     departure_source(dep_src_),
                                     cost_arrival(cst_arr_) {}
};
#endif //FLETCHER_STRUCTURES_HXX

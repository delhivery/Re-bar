//
// Created by amitprakash on 8/1/16.
//

#include "structures.hxx"

Cost::Cost(bool infinite) {
    if (infinite) {
        expense = std::numeric_limits<double>::infinity();
        duration = std::numeric_limits<long>::max();
    }
    else {
        expense = 0;
        duration = 0;
    }
}

bool Cost::is_infinite() const {
    if(expense == std::numeric_limits<double>::infinity()) {
        return true;
    }

    return duration == std::numeric_limits<long>::max();
}

bool operator < (const Cost& first, const Cost& second) {
    if (first.is_infinite()){
        return false;
    }

    if (second.is_infinite()) {
        return true;
    }

    if(first.time() < second.time()) {
        return true;
    }

    if(first.time() == second.time()) {
        return first.cost() < second.cost();
    }

    return false;
}

bool operator > (const Cost& first, const Cost& second) {
    return !(first < second);
}

Cost operator + (const Cost& first, const Cost& second) {
    if (first.is_infinite() or second.is_infinite()) {
        return Cost(true);
    }
    double time = first.time();
    time += second.time();

    if (time >= std::numeric_limits<long>::max()) {
        return Cost(true);
    }

    return Cost{first.cost() + second.cost(), first.time() + second.time()};
}

Cost& Cost::operator = (Cost operand) {
    std::swap(expense, operand.expense);
    std::swap(duration, operand.duration);
    return *this;
}

bool operator == (const Cost& first, const Cost& second) {
    if (first.is_infinite()) {
        return false;
    }

    if (second.is_infinite()) {
        return false;
    }

    return (first.cost() == second.cost()) and (first.time() == second.time());
}

#ifndef MARGE_HPP_INCLUDED
#define MARGE_HPP_INCLUDED

#include <mongo/reader.hpp>
#include <dijkstra.hpp>
#include <constrained.hpp>
#include <memory>

class Solver {
    private:
        std::string database = "rebar";
        std::string nodes_collection = "nnodes";
        std::string edges_collection = "nedges";
        std::shared_ptr<EPGraph> path_finder;
        int mode;

    public:
        Solver(std::string database="rebar", int mode=0);

        void init();

        std::vector<Path> solve(std::string origin, std::string destination, long dt_start, long dt_promise);

        std::tuple<std::shared_ptr<Connection>, std::shared_ptr<DeliveryCenter>, std::shared_ptr<DeliveryCenter> > lookup(std::string connection);
};

#endif

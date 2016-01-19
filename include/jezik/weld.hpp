#include <functional>
#include <experimental/any>

#include <marge/graph.hpp>
#include <marge/optimal.hpp>
#include <marge/pareto.hpp>

using namespace std;
using namespace std::experimental;


class Weld {
    private:
        static vector<shared_ptr<BaseGraph> > solvers;
        static map<string, function<json_map(shared_ptr<BaseGraph>, const map<string, any>&)> > welder;

    public:
        Weld();
        json_map operator() (int mode, string_view command, const map<string, any>& kwargs);
};

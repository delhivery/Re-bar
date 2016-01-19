#include <jezik/weld.hpp>
#include <future>

map<string, function<json_map(shared_ptr<BaseGraph>, const map<string, any>&)> > Weld::welder = {
    {"ADDV", BaseGraph::addv},
    {"ADDE", BaseGraph::adde},
    {"LOOK", BaseGraph::look},
    {"FIND", BaseGraph::find}
};

Weld::Weld() {
    solvers.push_back(make_shared<Pareto>());
    solvers.push_back(make_shared<Optimal>(true));
}

json_map Weld::operator () (int mode, string_view command, const map<string, any>& kwargs) {
    vector<json_map> responses;

    for(const auto& solver: solvers) {
        future<json_map> response = async(launch::async, [this, &command, &solver, &kwargs]{ return welder.at(command.to_string())(solver, kwargs); });
        responses.push_back(response.get());
    }

    return responses[mode];
}

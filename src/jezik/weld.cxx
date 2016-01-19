#include <jezik/weld.hpp>
#include <future>


map<string, function<json_map(shared_ptr<BaseGraph>, const map<string, any>&)> > Weld::welder = {
    {"ADDV", BaseGraph::addv},
    {"ADDE", BaseGraph::adde},
    {"LOOK", BaseGraph::look},
    {"FIND", BaseGraph::find}
};

Weld::Weld() {
    solvers.push_back(make_shared<BaseGraph>(Pareto{}));
    solvers.push_back(make_shared<BaseGraph>(Optimal{true}));
}

json_map Weld::operator () (int mode, string_view command, const map<string, any>& kwargs) {
    vector<json_map> responses;
    vector<future<json_map> > responses_async;

    for(const auto& solver: solvers) {
        future<json_map> response = async(launch::async, [this, &command, &solver, &kwargs]{ return welder.at(command.to_string())(solver, kwargs); });
        responses_async.push_back(response);
    }

    for (auto& response: responses_async) {
        responses.push_back(response.get());
    }

    return responses[mode];
}

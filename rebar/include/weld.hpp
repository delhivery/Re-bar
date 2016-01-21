#include <experimental/any>
#include <experimental/string_view>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <vector>
#include <jeayeson/jeayeson.hpp>

using namespace std;
using std::experimental::any;
using std::experimental::string_view;


template <typename T> class Weld {
    private:
        vector<shared_ptr<T> > solvers;
        static map<string, function<json_map(shared_ptr<T>, const map<string, any>&)> > welder;

    public:
        Weld() { }

        void add_solver(const shared_ptr<T>& solver) {
            solvers.push_back(solver);
        }

        json_map operator() (int mode, string_view command, const map<string, any>& kwargs) {
            vector<json_map> responses;

            for(const auto& solver: solvers) {
                future<json_map> response = async(launch::async, [this, &command, &solver, &kwargs]{ return welder.at(command.to_string())(solver, kwargs); });
                responses.push_back(response.get());
            }
            return responses[mode];
        }
};

template <typename T> map<string, function<json_map(shared_ptr<T>, const map<string, any>&)> > Weld<T>::welder = {
    {"ADDV", T::addv},
    {"ADDE", T::adde},
    {"LOOK", T::look},
    {"FIND", T::find}
};

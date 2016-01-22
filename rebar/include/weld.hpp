#include <experimental/any>
#include <experimental/string_view>
#include <functional>
#include <future>
#include <map>
#include <vector>
#include <jeayeson/jeayeson.hpp>

using namespace std;
using std::experimental::any;
using std::experimental::string_view;

/**
 * Class to handle command -> function mapping and invoke appropriate commands and return the results
 * Requires a template parameter to specify the type of supported solvers
 */
template <typename T> class Weld {

    private:
        /** Vector to maintain a list of solvers */
        vector<shared_ptr<T> > solvers;

        /** Map mapping a command to executable function against the solver */
        static map<string, function<json_map(shared_ptr<T>, const map<string, any>&)> > welder;

    public:
        /**
         * Default constructs a Weld instance
         */
        Weld() { }

        /**
         * Adds solvers to be used for commands
         * @param[in] solver: Shared pointer to a solver
         */
        void add_solver(const shared_ptr<T>& solver) {
            solvers.push_back(solver);
        }

        /**
         * Executes a command against solvers asynchronously, fetches the results and returns the mode appropriate solution
         * Returns a json response
         * @param [in] mode: Solver mode
         * @param [in] command: Command to execute
         * @param [in] kwargs: Named arguments for command
         */
        json_map operator() (int mode, string_view command, const map<string, any>& kwargs) {
            vector<json_map> data;
            vector<future<json_map> > responses;

            for (const auto& solver: solvers) {
                responses.push_back(
                    async(
                        [this, &command, &solver, &kwargs]() {
                            return welder.at(command.to_string())(solver, kwargs);
                        }
                    )
                );
            }

            for (auto& response: responses) {
                data.push_back(response.get());
            }

            return data[mode];
        }
};

template <typename T> map<string, function<json_map(shared_ptr<T>, const map<string, any>&)> > Weld<T>::welder = {
    {"ADDV", T::addv},
    {"ADDE", T::adde},
    {"ADDC", T::addc},
    {"LOOK", T::look},
    {"FIND", T::find}
};

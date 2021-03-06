/** @file weld.hpp
 * @brief Defines a class to map string commands to solver functions
 */
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
 * @brief Class to handle command -> function mapping
 * @details Stores command -> function mapping and invokes appropriate commands against solvers and returns their output.
 * Requires a template parameter to specify the type of supported solvers
 */
template <typename T> class Weld {

    private:
        /**
         * @brief Vector to maintain a list of solvers
         */
        vector<shared_ptr<T> > solvers;

        /**
         * @brief Map mapping a command to executable function against the solver
         */
        static map<string, function<json_map(shared_ptr<T>, const map<string, any>&)> > welder;

    public:
        /**
         * @brief Default constructs a Weld instance
         */
        Weld() { }

        /**
         * @brief Adds solvers to be used for commands
         * @param[in] solver: Shared pointer to a solver
         */
        void add_solver(const shared_ptr<T>& solver) {
            solvers.push_back(solver);
        }

        /**
         * @brief Executes a command against solvers asynchronously, fetches the results and returns the mode appropriate solution
         * @param[in] mode: Solver mode
         * @param[in] command: Command to execute
         * @param[in] kwargs: Named arguments for command
         * @return A json response as generated by command
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
    {"FIND", T::find},
    {"MODC", T::modc}
};

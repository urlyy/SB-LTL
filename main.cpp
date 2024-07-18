#include <iostream>
#include <vector>
#include <cassert>
#include <set>

#include <spot/tl/parse.hh>
#include <spot/tl/print.hh>
#include <spot/tl/exclusive.hh>
#include <spot/twaalgos/translate.hh>
#include <spot/twa/twagraph.hh>
#include <spot/twaalgos/sbacc.hh>
#include <spot/twaalgos/split.hh>
#include <spot/twa/bddprint.hh>

using std::vector;
using std::string;
using std::cout;
using spot::twa_graph_ptr;
using std::unordered_map;
using std::unordered_set;

unordered_map <string, int> bdd_map;
unordered_map<int, string> bdd_map_rev;

// global condition manager
unordered_map<string,bool> cond_sat;

// transform LTL formula to buchi automata
twa_graph_ptr formula2aut(const string& formula){
    cout<<"==================== formula to automata"<<'\n';
    cout<<"LTL is "<<formula<<std::endl;
    spot::parsed_formula pf = spot::parse_infix_psl(formula);
    std::ostringstream error;
    if (pf.format_errors(error)) {
        throw std::runtime_error("Failed to parse formula " + formula + '\n' + error.str());
    }
    twa_graph_ptr automata;
    spot::translator translator;
    // shouldn't use sbacc
    translator.set_type(spot::postprocessor::Buchi);
    translator.set_pref(spot::postprocessor::Complete | spot::postprocessor::Deterministic);
    // Below will split a "true" trans to "!a" and "a",two edges
    // aut = spot::split_edges(aut);
    return translator.run(pf.f);
}

// extrace bdd_map from buchi
// used for check if can trans
void init(twa_graph_ptr &aut){
    vector<spot::formula> ap_formulas;
    cout<<"==================== ap in LTL"<<'\n';
    for (const auto &kv : aut->get_dict()->var_map) {
        bdd_map[kv.first.ap_name()] = kv.second;
        bdd_map_rev[kv.second] = kv.first.ap_name();
        cout<<kv.first.ap_name()<<": "<<kv.second<<"\n";
        /**
         * Initialize all conditions involved in LTL to false, 
         * indicating they are not satisfied.
         */
        cond_sat[kv.first.ap_name()] = false;
    }
}

void print_aut(twa_graph_ptr aut){
    cout<<"============================= Automata"<<'\n';
    unsigned init = aut->get_init_state_number();
    cout << "Initial state:";
    if (aut->is_univ_dest(init))
    cout << " {";
    for (unsigned i: aut->univ_dests(init))
    cout << ' ' << i;
    if (aut->is_univ_dest(init))
    cout << " }";
    cout << '\n';
    const spot::bdd_dict_ptr& dict = aut->get_dict();
    unsigned n = aut->num_states();
    for (unsigned s = 0; s < n; ++s)
    {
        cout << "State " << s << ":\n";
        for (auto& t: aut->out(s))
        {
            cout << "  edge(" << t.src << " ->";
            if (aut->is_univ_dest(t))
            cout << " {";
            for (unsigned dst: aut->univ_dests(t))
            cout << ' ' << dst;
            if (aut->is_univ_dest(t))
            cout << " }";
            cout << ")\n    label = ";
            cout << spot::bdd_format_formula(dict, t.cond);
            cout << "\n    acc sets = " << t.acc << '\n';
        }
    }
    cout << "accept state ID: { ";
    const spot::acc_cond& acc = aut->acc();
    unsigned ns = aut->num_states();
    for (unsigned s = 0; s < ns; ++s){
        if (acc.accepting(aut->state_acc_sets(s))){
            cout << s << ',';
        }
    }
    std::cout << " }\n";
}

bool cond_check_res;
// check if this trans condition is "true"
static void true_cond_checker(char *cond, int size)
{
    // no relavant condition,means "true"
    for (int i = 0; i < size; i++) {
        if (cond[i] >= 0) {
            cond_check_res = false;
            return;
        }
    }
    cond_check_res = true;
}

// check if the global condition satisfy current edge's trans condition
static void cond_checker(char *cond, int size)
{
    int match = 0;
    int total = 0;
    cout<<"cond_check: (";
    for(const auto &kv:cond_sat){
        // cond[idx],idx is a number of ap,in bdd_map
        const int idx = bdd_map[kv.first];
        // -1 means it does not matter.
        if (cond[idx] == -1) {
            continue;
        }
        total++;
        // You can see detailed conditions by print
        cout<<bdd_map_rev[idx]<<":"<<(int)cond[idx]<<", ";
        // this global condition is true, and edge condition is true
        if (cond[idx] == 1 && kv.second) {
            match++;
        }
        // this global condition is false, and edge condition is false
        if (cond[idx] == 0 && !kv.second) {
            match++;
        }
    }
    cout<<")\n";
    cond_check_res = match == total;
}

// return if changes
bool change_cond(const string cond){
    if(cond[0] == '!'){
        const string true_cond = cond.substr(1);
        if(!cond_sat[true_cond]){
            return false;
        }
        cond_sat[true_cond] = false;
        return true;
    }else{
         if(cond_sat[cond]){
            return false;
        }
        cond_sat[cond] = true;
        return true;
    }
}

struct State{
    int num;
    int accept;
};

// Get the corresponding transfrom trace on automata of events
void getTrace(const twa_graph_ptr &aut,const vector<string>& events,vector<State> &states){
    cout<<"============================= getTrace"<<'\n';
    unsigned cur_state_id = aut->get_init_state_number();
    State init = {(int)cur_state_id,aut->state_is_accepting(cur_state_id)};
    states.push_back(init);
    for(int i=0;i<events.size();i++){
        string e = events[i];
        string ee = e;
        if(ee[0]=='!'){
            ee = ee.substr(1);
        }
        // Ignore events that are irrelevant to LTL.
        bool exist = bdd_map.find(ee)!=bdd_map.end();
        if(!exist)continue;
        // change global conditions
        change_cond(e);
        cout<<"======== cur_state_id: "<<cur_state_id<<" , cur_event: " << e << "\n";
        bool sat = false;
        // Traverse all the outgoing edges from the current state.
        for (auto& edge: aut->out(cur_state_id)){
            cout<<edge.src << "->" << edge.dst<<" trans:" << spot::bdd_format_formula(aut->get_dict(), edge.cond)<<"\n";
            // bdd_allsat document reference：https://buddy.sourceforge.net/manual/group__operator_gf41487d86f76d3480d379ff434a2c476.html#gf41487d86f76d3480d379ff434a2c476
            bdd_allsat(edge.cond,true_cond_checker);
            // trans condition is "true"
            // current status node only has one out edge
            if(cond_check_res){
                cur_state_id = edge.dst;
                sat = true;
                break;
            }
            bdd_allsat(edge.cond, cond_checker);
            if(cond_check_res){
                cur_state_id = edge.dst;
                sat = true;
                break;
            }
        }
        if(sat){
            const bool accept = aut->state_is_accepting(cur_state_id);
            const State state = {(int)cur_state_id,accept};
            states.push_back(state);
        }else{
            states.push_back({-1,false});
            break;
        }
    }
}

void extract_apath(const vector<State> states,vector<unsigned int> &aPath){
    cout<<"============================= extract_prefix_apath"<<'\n';
    int cur_state_id = -2;
    int last_idx = -1;
    for(int i=0;i<states.size();i++){
        const State state = states[i];
        if(state.num == cur_state_id)continue;
        cur_state_id = state.num;
        last_idx = i;
        aPath.push_back(state.num);
    }
}

int main()
{
    string formula = "F(a & F(b))";
    vector<string> events = {"c","!a","a","a","a","c","b","c","a","c"};
    twa_graph_ptr aut = formula2aut(formula);
    init(aut);
    print_aut(aut);
    vector<State> states;
    getTrace(aut,events,states);
    for(auto state:states){
        cout<<state.num<<" "<<state.accept<<"\n";
    }
    // true transfrom path in automata
    vector<unsigned int> aPath;
    extract_apath(states,aPath);
    cout<< "aPath: ";
    for(unsigned p : aPath){
        cout<<p<<" > ";
    }
    cout<<"\n";
    bool res = states[states.size()-1].accept;
    string s = res?"true":"false";
    cout<<"accept: "<<s<<'\n';   
    return 0;
}
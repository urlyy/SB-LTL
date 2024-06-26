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


bool is_output(const string &e){
    return e[0]=='o';
}

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
    // 不能用sbacc
    translator.set_type(spot::postprocessor::Buchi);
    translator.set_pref(spot::postprocessor::Complete | spot::postprocessor::Deterministic);
    return translator.run(pf.f);
}

void init(twa_graph_ptr &aut){
    
    vector<spot::formula> ap_formulas;
    /**
     * 把公式里的ap提取出来并从0开始编号
     * Construct BDD variable map
     */
    cout<<"==================== ap in LTL"<<'\n';
    for (const auto &kv : aut->get_dict()->var_map) {
        bdd_map[kv.first.ap_name()] = kv.second;
        bdd_map_rev[kv.second] = kv.first.ap_name();
        cout<<kv.first.ap_name()<<": "<<kv.second<<"\n";
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
    // 下面这个可要可不要
    // if (!(acc.is_buchi() || acc.is_t()))
    //     throw std::runtime_error("unsupported acceptance condition");
    // // The BA format only support state-based acceptance, so get rid
    // // of transition-based acceptance if we have some.
    // // aut = spot::sbacc(aut);
    // // 这样会把一个true拆开成正和非正两个迁移
    // // aut = spot::split_edges(aut);
    // std::cout << "Initial state: " << aut->get_init_state_number() << '\n';
    // const spot::bdd_dict_ptr& dict = aut->get_dict();
    // unsigned n = aut->num_states();
    // for (unsigned s = 0; s < n; ++s)
    // {
    //     std::cout << "State " << s << ":\n";
    //     for (auto& t: aut->out(s))
    //     {
    //         std::cout << "  edge(" << t.src << " -> " << t.dst << ")\n    label = ";
    //         std::cout << spot::bdd_format_formula(dict, t.cond);
    //         std::cout << "\n    acc sets = " << t.acc << '\n';
    //     }
    // }
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
// 判断这个trans的条件是否为1(TRUE)
static void true_cond_checker(char *cond, int size)
{
    // 没有相关的命题，即为一个true
    for (int i = 0; i < size; i++) {
        if (cond[i] >= 0) {
            cond_check_res = false;
            return;
        }
    }
    cond_check_res = true;
}

unordered_set<int> cur_events_idx;
// 判断这个trans的条件是否是本次事件的子集
static void cond_checker(char *cond, int size)
{
    int n_true = 0;
    for (int i = 0; i < size; i++) {
        if (cond[i] == 1) {
            n_true++;
        }
    }
    int n_match = 0;
    for(int idx:cur_events_idx){
        // 跳过不在LTL里的事件
        if(idx != -1){
            // 其实cond的每个下标，就是bdd_map里，和LTL有关的ap的每个编号
            // -1表示不关心
            if (cond[idx] == -1) {
                continue;
            }
            cout<<"cond_check: ("<<bdd_map_rev[idx]<<":"<<(int)cond[idx]<<")"<<"\n";
            // 命题为假,直接跳出
            if (cond[idx] == 0) {
                cond_check_res = false;
                cout<<'\n';
                return;
            }
            // 命题为真
            if (cond[idx] == 1) {
                n_match++;
            }
        }
    }
    cout<<'\n';
    assert(n_match <= n_true);
    cond_check_res = n_match == n_true;
}

bool check_accept(const twa_graph_ptr &aut,const vector<unordered_set<string>>& all_events){
    cout<<"============================= Check"<<'\n';
    unsigned cur_state_id = aut->get_init_state_number();
    // 用来判断是否可以提前结束
    // 即如果到达出边的trans为1时，可以提前结束
    bool reach_end = false;
    for(const unordered_set<string>& cur_events:all_events){
        cur_events_idx.clear();
        // 不在LTL里的一律为-1
        for(string e:cur_events){
            if(bdd_map.find(e)==bdd_map.end()){
                cur_events_idx.insert(-1);
            }else{
                cur_events_idx.insert(bdd_map[e]);
            }
        }
        cout<<"cur_state_id: "<<cur_state_id<<" , cur_event: [";
        for(auto e:cur_events){
            cout<<e<<",";
        } 
        cout<<"]\n";
        bool sat = false;
        // 遍历当前状态所有的出边
        for (auto& edge: aut->out(cur_state_id)){
            cout<<edge.src << "->" << edge.dst<<" trans:" << spot::bdd_format_formula(aut->get_dict(), edge.cond)<<"\n";
            // bdd_allsat的文档参考：https://buddy.sourceforge.net/manual/group__operator_gf41487d86f76d3480d379ff434a2c476.html#gf41487d86f76d3480d379ff434a2c476
            // 似乎内部会多次调用回调函数，具体看例子 LTL="i1 & X(o2 & !o3)"; inputs = {{"i1"},{"o2","o3"},{"i2"}};
            bdd_allsat(edge.cond,true_cond_checker);
            // 转移条件是true
            if(cond_check_res){
                cur_state_id = edge.dst;
                sat = true;
                // 提前结束
                if(edge.src==edge.dst){
                    reach_end = true;
                }
                break;
            }
            bdd_allsat(edge.cond, cond_checker);
            // 当前命题满足转移
            if(cond_check_res){
                cur_state_id = edge.dst;
                sat = true;
                break;
            }
        }
        if(!sat || reach_end){
            break;
        }
    }
    cout<<"final_state_id: "<<cur_state_id<<'\n';
    bool accepting = aut->state_is_accepting(cur_state_id);
    return accepting;
}

int main()
{
    /* string formula = "i1 -> o2";   */
    // 输入只能是 ixx或oxx，不能有!ixx/!oxx这种。可以将o3认为是!o2
    // string formula = "!i1 || (i1 & X(o2))";
    // string formula1 = "i1 & F(o2)";
    // string formula2 = "i1 & X(o2)";
    // string formula3 = "F(i1 & X(o2))";
    string formula4 = "i1 & X(o2 & !o3)";
    string formula5 = "F(i1 & X(o2 & !o3))";
    // 程序的输入输出的trace序列。偶数是输入，奇数是输出
    vector<unordered_set<string>> inputs = {{"i1"},{"o2","o3"},{"i1"},{"o2","o4"},{"i1"},{"o2","o3"}};
    twa_graph_ptr aut = formula2aut(formula5);
    init(aut);
    print_aut(aut);
    bool res = check_accept(aut,inputs);
    string s = res?"true":"false";
    cout<<"accept: "<<s<<'\n';   
    return 0;
}
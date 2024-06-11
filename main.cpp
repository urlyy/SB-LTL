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

std::unordered_map<string, int> bdd_map_;

bool is_output(const string &e){
    return e[0]=='o';
}

void print_aut(twa_graph_ptr aut){
    unsigned init = aut->get_init_state_number();
    std::cout << "Initial state:";
    if (aut->is_univ_dest(init))
    std::cout << " {";
    for (unsigned i: aut->univ_dests(init))
    std::cout << ' ' << i;
    if (aut->is_univ_dest(init))
    std::cout << " }";
    std::cout << '\n';
    const spot::bdd_dict_ptr& dict = aut->get_dict();
    unsigned n = aut->num_states();
    for (unsigned s = 0; s < n; ++s)
    {
        std::cout << "State " << s << ":\n";
        for (auto& t: aut->out(s))
        {
            std::cout << "  edge(" << t.src << " ->";
            if (aut->is_univ_dest(t))
            std::cout << " {";
            for (unsigned dst: aut->univ_dests(t))
            std::cout << ' ' << dst;
            if (aut->is_univ_dest(t))
            std::cout << " }";
            std::cout << ")\n    label = ";
            spot::bdd_print_formula(std::cout, dict, t.cond);
            std::cout << "\n    acc sets = " << t.acc << '\n';
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
    cout << "接收结点ID: { ";
    const spot::acc_cond& acc = aut->acc();
    unsigned ns = aut->num_states();
    for (unsigned s = 0; s < ns; ++s){
        if (acc.accepting(aut->state_acc_sets(s))){
            cout << s << ',';
        }
    }
    std::cout << " }\n";
}

void init(twa_graph_ptr &aut, const vector<string>& inputs){
    const std::set<string> aps(inputs.begin(), inputs.end());
    vector<spot::formula> ap_formulas;
     /*
     * 把每个ap变成公式
     * 照着ltlfuzzer写的，我也不知道在干嘛
     * 好像还会造成问题?
     */
    for(const string& ap:aps){
        ap_formulas.push_back(spot::parse_infix_psl(ap).f);
    }
    if (!ap_formulas.empty()) {
        spot::exclusive_ap excl;
        excl.add_group(ap_formulas);
        aut = excl.constrain(aut, true);
    }
    /**
     * 把公式里的ap提取出来并从0开始编号
     * Construct BDD variable map
     */
    bdd_map_.clear();
    int nex_id = 0;
    // reverse_bdd_map_.clear();
    for (const auto &kv : aut->get_dict()->var_map) {
        bdd_map_[kv.first.ap_name()] = kv.second;
        cout<<kv.first.ap_name()<<": "<<kv.second<<"\n";
        nex_id = kv.second + 1;
        // reverse_bdd_map_[kv.second] = kv.first.ap_name();
    }
    /**
     * 为了使用到公式外的ap，我再额外添加一次
     * 但这样也有如果输入太分散导致大bdd_map太大的问题
     * NEW: 只加输出对应的ap
     */
    for(const string& ap:aps){
        if(bdd_map_.find(ap) == bdd_map_.end() && is_output(ap)){
            bdd_map_[ap] = nex_id;
            nex_id ++;
        }
    }
}

twa_graph_ptr formula2aut(const string& formula){
    spot::parsed_formula pf = spot::parse_infix_psl(formula);
    std::ostringstream error;
    if (pf.format_errors(error)) {
        throw std::runtime_error("Failed to parse formula " + formula + '\n' + error.str());
    }
    twa_graph_ptr automata;
    spot::translator translator;
    // 注意这里没有用Deterministic
    translator.set_pref(spot::postprocessor::Complete | spot::postprocessor::SBAcc | spot::postprocessor::Deterministic);
    automata = translator.run(pf.f);
    return automata;
}

bool cond_check_res;
static void true_cond_checker(char *cond, int size)
{
    // 全部都不满足即true
    for (int i = 0; i < size; i++) {
        if (cond[i] >= 0) {
            cond_check_res = false;
            return;
        }
    }
    cond_check_res = true;
}


// 其实cond的每个下标，就是bdd_map里，和LTL有关的ap的每个编号
int event_idx;
static void cond_checker(char *cond, int size)
{
    int n_true = 0;
    for (int i = 0; i < size; i++) {
        if (cond[i] == 1) {
            n_true++;
        }
    }
    int n_match = 0;
    // ap未在LTL中时，-1表示不关心,这里我们无需处理
    if(event_idx < size){
        // 命题为假,直接跳出
        if (cond[event_idx] == 0) {
            cond_check_res = false;
            return;
        }
        // 命题为真
        if (cond[event_idx] == 1) {
            n_match++;
        }
    }
    assert(n_match <= n_true);
    // 只有一个命题能为真
    cond_check_res = n_match == n_true;
}



bool check_accept(const twa_graph_ptr &aut,const vector<string>& events){
    unsigned cur_state_id = aut->get_init_state_number();
    for(const string& e:events){
        // 其实这里只会跳过没在LTL里的输入
        if(bdd_map_.find(e) == bdd_map_.end()){
            continue;
        }
        event_idx = bdd_map_[e];
        cout<<"event: "<<e<<" ,cur_state_id: "<<cur_state_id<<"\n";
        bool sat = false;
        // 遍历当前状态所有的出边
        for (auto& edge: aut->out(cur_state_id)){
            cout<<edge.src << " " << edge.dst<<" " << spot::bdd_format_formula(aut->get_dict(), edge.cond)<<"\n";
            // std::cout << "\n    acc sets = " << e.acc << '\n';
            // bdd_allsat的文档参考：https://buddy.sourceforge.net/manual/group__operator_gf41487d86f76d3480d379ff434a2c476.html#gf41487d86f76d3480d379ff434a2c476
            bdd_allsat(edge.cond,true_cond_checker);
            // 转移条件是true
            if(cond_check_res){
                cur_state_id = edge.dst;
                sat = true;
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
        if(!sat){
            break;
        }
        // 这个貌似有问题，不能用前缀处理
        // bool accepting = aut->state_is_accepting(cur_state_id);
        // if(accepting){
        //     return true;
        // }
    }
    bool accepting = aut->state_is_accepting(cur_state_id);
    return accepting;
}

int main()
{
    /* string formula = "i1 -> o2";   */
    // 因为是单个单个，所以不支持同时两个ap的判断
    // !i1 || o2，即要不i1不发生，要不i1和o2都发生
    // 上面这种还是改成下面这种比较好
    string formula = "!i1 || (i1 & X(o2))";
    string formula1 = "i1 & F(o2)";
    string formula2 = "i1 & X(o2)";
    // 程序的输入输出的trace序列
    vector<string> inputs = {"i2","o2","o2"};
    twa_graph_ptr aut = formula2aut(formula);
    init(aut,inputs);
    print_aut(aut);
    bool res = check_accept(aut,inputs);
    string s = res?"true":"false";
    cout<<"accept: "<<s<<'\n';   
    return 0;
}
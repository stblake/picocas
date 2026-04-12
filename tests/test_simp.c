#include "test_utils.h"
#include "symtab.h"
#include "core.h"

void test_trigtoexp() {
    assert_eval_eq("TrigToExp[Cos[x]]", "1/2 E^((-I) x) + 1/2 E^((I) x)", 0);
    assert_eval_eq("TrigToExp[Sinh[x]]", "1/2 E^x - 1/2 E^(-1 x)", 0);
    assert_eval_eq("TrigToExp[Cos[x]+I Sin[x]]", "E^((I) x)", 0);
    assert_eval_eq("TrigToExp[Tanh[x]]", "-E^(-1 x)/(E^x + E^(-1 x)) + E^x/(E^x + E^(-1 x))", 0);
    assert_eval_eq("TrigToExp[ArcSin[x]]", "(-I) Log[(I) x + Sqrt[1 - x^2]]", 0);
    assert_eval_eq("TrigToExp[ArcCoth[x]]", "-1/2 Log[1 - 1/x] + 1/2 Log[1 + 1/x]", 0);
    assert_eval_eq("TrigToExp[{Sin[x],Cos[x],Tan[x]}]", "{(-1/2*I) E^((I) x) + (1/2*I) E^((-I) x), 1/2 E^((-I) x) + 1/2 E^((I) x), (-I) E^((I) x)/(E^((-I) x) + E^((I) x)) + (I) E^((-I) x)/(E^((-I) x) + E^((I) x))}", 0);
    assert_eval_eq("TrigToExp[Sinh[x]<=11&&ArcTan[x]==7]", "1/2 E^x - 1/2 E^(-1 x) <= 11 && (-1/2*I) Log[1 + (I) x] + (1/2*I) Log[1 + (-I) x] == 7", 0);

    // Other inverse trig
    assert_eval_eq("TrigToExp[ArcCos[x]]", "1/2 Pi + (I) Log[(I) x + Sqrt[1 - x^2]]", 0);
    assert_eval_eq("TrigToExp[ArcCot[x]]", "(-1/2*I) Log[1 + (I)/x] + (1/2*I) Log[1 + (-I)/x]", 0);
    assert_eval_eq("TrigToExp[ArcSec[x]]", "1/2 Pi + (I) Log[(I)/x + Sqrt[1 - 1/x^2]]", 0);
    assert_eval_eq("TrigToExp[ArcCsc[x]]", "(-I) Log[(I)/x + Sqrt[1 - 1/x^2]]", 0);
    
    // Hyperbolics
    assert_eval_eq("TrigToExp[Cosh[x]]", "1/2 E^x + 1/2 E^(-1 x)", 0);
    assert_eval_eq("TrigToExp[Coth[x]]", "E^x/(-E^x + E^(-1 x)) + E^(-1 x)/(-E^x + E^(-1 x))", 0);
    assert_eval_eq("TrigToExp[Sech[x]]", "2/(E^x + E^(-1 x))", 0);
    assert_eval_eq("TrigToExp[Csch[x]]", "2/(-E^(-1 x) + E^x)", 0);

    // Inverse hyperbolics
    assert_eval_eq("TrigToExp[ArcSinh[x]]", "Log[x + Sqrt[1 + x^2]]", 0);
    assert_eval_eq("TrigToExp[ArcCosh[x]]", "Log[x + Sqrt[-1 + x] Sqrt[1 + x]]", 0);
    assert_eval_eq("TrigToExp[ArcTanh[x]]", "1/2 Log[1 + x] - 1/2 Log[1 - x]", 0);
    assert_eval_eq("TrigToExp[ArcSech[x]]", "Log[1/x + Sqrt[-1 + 1/x] Sqrt[1 + 1/x]]", 0);
    assert_eval_eq("TrigToExp[ArcCsch[x]]", "Log[1/x + Sqrt[1 + 1/x^2]]", 0);
    
    // Two-argument ArcTan
    assert_eval_eq("TrigToExp[ArcTan[x, y]]", "(-I) Log[(x + (I) y)/Sqrt[x^2 + y^2]]", 0);
}

int main() {
    symtab_init();
    core_init();
    test_trigtoexp();
    printf("All simp tests passed!\n");
    return 0;
}

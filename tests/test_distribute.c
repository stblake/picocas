#include "test_utils.h"
#include "symtab.h"
#include "core.h"
#include <stdio.h>

void test_distribute() {
    // Apply the distributive law:
    assert_eval_eq("Distribute[(a+b).(x+y+z)]", "Dot[a, x] + Dot[a, y] + Dot[a, z] + Dot[b, x] + Dot[b, y] + Dot[b, z]", 0);
    
    // Distribute f over Plus:
    assert_eval_eq("Distribute[f[a+b,c+d+e]]", "f[a, c] + f[a, d] + f[a, e] + f[b, c] + f[b, d] + f[b, e]", 0);
    
    // Distribute f over g:
    assert_eval_eq("Distribute[f[g[a,b],g[c,d,e]],g]", "g[f[a, c], f[a, d], f[a, e], f[b, c], f[b, d], f[b, e]]", 0);
    
    // By default, distribute over Plus:
    assert_eval_eq("Distribute[(a+b+c)(u+v),Plus]", "a u + a v + b u + b v + c u + c v", 0);
    assert_eval_eq("Distribute[(a+b+c)(u+v)]", "a u + a v + b u + b v + c u + c v", 0);
    
    // Distribute only a product of sums:
    assert_eval_eq("Distribute[(a+b+c)(u+v),Plus,Times]", "a u + a v + b u + b v + c u + c v", 0);
    assert_eval_eq("Distribute[(a+b+c)^(u+v),Plus,Times]", "(a + b + c)^(u + v)", 0);
    
    // Distribute f over g to give fp and gp:
    assert_eval_eq("Distribute[f[g[a,b],g[c,d,e]],g,f,gp,fp]", "gp[fp[a, c], fp[a, d], fp[a, e], fp[b, c], fp[b, d], fp[b, e]]", 0);
    assert_eval_eq("Distribute[(a+b+c)(u+v),Plus,Times,plus,times]", "plus[times[u, a], times[u, b], times[u, c], times[v, a], times[v, b], times[v, c]]", 0);
    
    // Expand symbolic matrix and vector expressions:
    assert_eval_eq("Distribute[(m1+m2).(m3+m4)]", "Dot[m1, m3] + Dot[m1, m4] + Dot[m2, m3] + Dot[m2, m4]", 0);
    assert_eval_eq("Distribute[Cross[v1+v2, v3+v4]]", "Cross[v1, v3] + Cross[v1, v4] + Cross[v2, v3] + Cross[v2, v4]", 0);
    
    // Find the list of all possible combinations of elements:
    assert_eval_eq("Distribute[{{a, b}, {x,y,z},{s,t}}, List]", "{{a, x, s}, {a, x, t}, {a, y, s}, {a, y, t}, {a, z, s}, {a, z, t}, {b, x, s}, {b, x, t}, {b, y, s}, {b, y, t}, {b, z, s}, {b, z, t}}", 0);
    
    // Generate a powerset:
    assert_eval_eq("Distribute[{{{},{a}}, {{},{b}}, {{},{c}}}, List, List, List, Join]", "{{}, {c}, {b}, {b, c}, {a}, {a, c}, {a, b}, {a, b, c}}", 0);

    // Find intermediate terms from a direct application of the distributive law:
    assert_eval_eq("Distribute[(-1 + x) (1 + x) (1 - x + x^2) (1 + x + x^2), Plus, Times, List, Times]", 
        "{-1, x, -1 x^2, -1 x, x^2, -1 x^3, -1 x^2, x^3, -1 x^4, -1 x, x^2, -1 x^3, -1 x^2, x^3, -1 x^4, -1 x^3, x^4, -1 x^5, x, -1 x^2, x^3, x^2, -1 x^3, x^4, x^3, -1 x^4, x^5, x^2, -1 x^3, x^4, x^3, -1 x^4, x^5, x^4, -1 x^5, x^6}", 0);

    // For pure products, Distribute gives the same results as Expand:
    assert_eval_eq("Distribute[(-1 + x) (1 + x) (1 - x + x^2 - x^3 + x^4) (1 + x + x^2 + x^3 + x^4)]", "-1 + x^10", 0);
}

int main() {
    symtab_init();
    core_init();
    test_distribute();
    printf("All distribute tests passed!\\n");
    return 0;
}

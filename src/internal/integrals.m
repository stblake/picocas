
(* CRC Tables *)

Integrate[a_, x_] /; FreeQ[a, x] := a x;

Integrate[a_ f_, x_] /; FreeQ[a, x] := a Integrate[f, x];
Integrate[f_ + g_, x_] := Integrate[f, x] + Integrate[g, x];
Integrate[x_^n_., x_] /; FreeQ[n, x] && n =!= -1 := x^(n + 1)/(n + 1);
Integrate[1/x_, x_] := Log[x];
Integrate[Exp[a_. x_], x_] /; FreeQ[a, x] := Exp[a x]/a;
Integrate[b_^(a_. x_), x_] /; FreeQ[{a, b}, x] && b > 0 := b^(a x)/(a Log[b]);
Integrate[Log[a_. x_], x_] /; FreeQ[a, x] := x Log[a x] - x;
Integrate[1/(x_^2 + a_), x_] /; FreeQ[a, x] && a > 0 := 1/Sqrt[a] ArcTan[x/Sqrt[a]]; 
Integrate[1/(a_ - x_^2), x_] /; FreeQ[a, x] && a > 0 := 1/Sqrt[a] ArcTanh[x/Sqrt[a]];
Integrate[1/(x_^2 + a_), x_] /; FreeQ[a, x] && a < 0 := -1/Sqrt[-a] ArcCoth[x/Sqrt[-a]];
Integrate[1/Sqrt[a_ - x_^2], x_] /; FreeQ[a, x] && a > 0 := ArcSin[x/Sqrt[a]]
SetAttributes[Integrate, {Protected, ReadProtected}]
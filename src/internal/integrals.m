
(* Source: CRC Standard Mathematical Tables, 31st Edition *)

(* Elementary forms *)
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
Integrate[Power[a_ - x_^2, -1/2], x_] /; FreeQ[a, x] && a > 0 := ArcSin[x/Sqrt[a]];
Integrate[Power[x_^2 + a_, -1/2], x_] /; FreeQ[a, x] := Log[x + Sqrt[x^2 + a]];
Integrate[Power[x_^2 - a_, -1/2], x_] /; FreeQ[a, x] := Log[x + Sqrt[x^2 - a]];
Integrate[1/x_ Power[x_^2 - a_, -1/2], x_] /; FreeQ[a, x] := ArcTan[Sqrt[x^2 - a]/Sqrt[a]]/Sqrt[a];
Integrate[1/x_ Power[b_. x_^2 + a_, -1/2], x_] := -ArcTanh[Sqrt[b x^2 + a]/Sqrt[a]]/Sqrt[a];

(* Forms containing a + b x *)
(* Formula 23 *)
Integrate[(b_. x_ + a_.)^n_, x_] /; FreeQ[{a, b, n}, x] && n =!= -1 := ((a + b x)^(n + 1))/(b*(n + 1));

(* Formulae 24 *)
Integrate[x_ (b_. x_ + a_.)^n_, x_] /; FreeQ[{a, b, n}, x] && n =!= -1 && n =!= -2 := (b x + a)^(n + 2)/(b^2 (n + 2)) - a (a + b x)^(n + 1)/(b^2 (n + 1));

(* Formula 25 *)
Integrate[x_^2 (b_. x_ + a_.)^n_, x_] /; FreeQ[{a, b, n}, x] && n =!= -1 && n =!= -2 && n =!= -3 := 
  1/b^3 ((a + b x)^(n + 3)/(n + 3) - 2 a (a + b x)^(n + 2)/(n + 2) + a^2 (a + b x)^(n + 1)/(n + 1));

(* Formula 26: Three distinct reduction forms are provided. *)
(* Form 1: Reduces the power of the binomial *)
Integrate[x_^m_ (b_. x_ + a_.)^n_, x_] /; FreeQ[{a, b, m, n}, x] && m + n + 1 =!= 0 := 
  (x^(m + 1) (a + b x)^n)/(m + n + 1) + (a n)/(m + n + 1) Integrate[x^m (a + b x)^(n - 1), x];

(* Form 2: Increases the power of the binomial *)
Integrate[x_^m_ (b_. x_ + a_.)^n_, x_] /; FreeQ[{a, b, m, n}, x] && n =!= -1 := 
  1/(a (n + 1)) (-x^(m + 1) (a + b x)^(n + 1) + (m + n + 2) Integrate[x^m (a + b x)^(n + 1), x]);

(* Form 3: Reduces the power of x *)
Integrate[x_^m_ (b_. x_ + a_.)^n_, x_] /; FreeQ[{a, b, m, n}, x] && m + n + 1 =!= 0 := 
  1/(b (m + n + 1)) (x^m (a + b x)^(n + 1) - m a Integrate[x^(m - 1) (a + b x)^n, x]);

(* Formula 27 *)
Integrate[1/(a_ + b_. x_), x_] /; FreeQ[{a, b}, x] := 1/b Log[a + b x];

(* Formula 28 *)
Integrate[1/(a_ + b_. x_)^2, x_] /; FreeQ[{a, b}, x] := -1/(b (a + b x));

(* Formula 29 *)
Integrate[1/(a_ + b_. x_)^3, x_] /; FreeQ[{a, b}, x] := -1/(2 b (a + b x)^2);

(* Formula 30 *)
Integrate[x_/(a_ + b_. x_), x_] /; FreeQ[{a, b}, x] := 1/b^2 (a + b x - a Log[a + b x]);
  (* Note: The alternative form x/b - a/b^2 Log[a + b x] is mathematically equivalent *)

(* Formula 31 *)
Integrate[x_/(a_ + b_. x_)^2, x_] /; FreeQ[{a, b}, x] := 1/b^2 (Log[a + b x] + a/(a + b x));

(* Formula 32 *)
Integrate[x_/(a_ + b_. x_)^n_, x_] /; FreeQ[{a, b, n}, x] && n =!= 1 && n =!= 2 := 1/b^2 (-1/((n - 2) (a + b x)^(n - 2)) + a/((n - 1) (a + b x)^(n - 1)));

(* Formula 33 *)
Integrate[x_^2/(a_ + b_. x_), x_] /; FreeQ[{a, b}, x] := 1/b^3 (1/2 (a + b x)^2 - 2 a (a + b x) + a^2 Log[a + b x]);

(* Formula 34 *)
Integrate[x_^2/(a_ + b_. x_)^2, x_] /; FreeQ[{a, b}, x] := 1/b^3 (a + b x - 2 a Log[a + b x] - a^2/(a + b x));

(* Formula 35 *)
Integrate[x_^2/(a_ + b_. x_)^3, x_] /; FreeQ[{a, b}, x] := 1/b^3 (Log[a + b x] + 2 a/(a + b x) - a^2/(2 (a + b x)^2));

(* Formula 36 *)
Integrate[x_^2/(a_ + b_. x_)^n_, x_] /; FreeQ[{a, b, n}, x] && n =!= 1 && n =!= 2 && n =!= 3 := 1/b^3 (-1/((n - 3) (a + b x)^(n - 3)) + 2 a/((n - 2) (a + b x)^(n - 2)) - a^2/((n - 1) (a + b x)^(n - 1)));

(* Formula 37 *)
Integrate[1/(x_ (a_ + b_. x_)), x_] /; FreeQ[{a, b}, x] := 1/a Log[x/(a + b x)];

(* Formula 38 *)
Integrate[1/(x_ (a_ + b_. x_)^2), x_] /; FreeQ[{a, b}, x] := 1/(a (a + b x)) - 1/a^2 Log[(a + b x)/x];

(* Formula 39 *)
Integrate[1/(x_ (a_ + b_. x_)^3), x_] /; FreeQ[{a, b}, x] := 1/a^3 (1/2 ((2 a + b x)/(a + b x))^2 - Log[(a + b x)/x]);

(* Formula 40 *)
Integrate[1/(x_^2 (a_ + b_. x_)), x_] /; FreeQ[{a, b}, x] := -1/(a x) + b/a^2 Log[(a + b x)/x];

(* Formula 41 *)
Integrate[1/(x_^3 (a_ + b_. x_)), x_] /; FreeQ[{a, b}, x] := (2 b x - a)/(2 a^2 x^2) + b^2/a^3 Log[x/(a + b x)];

(* Formula 42 *)
Integrate[1/(x_^2 (a_ + b_. x_)^2), x_] /; FreeQ[{a, b}, x] := -(a + 2 b x)/(a^2 x (a + b x)) + (2 b)/a^3 Log[(a + b x)/x];

(* --- Section 5.4.3: Forms containing c^2 ± x^2 and x^2 - c^2 --- *)

(* Formula 43 *)
Integrate[1/(c_^2 + x_^2), x_] /; FreeQ[c, x] := 1/c ArcTan[x/c];

(* Formula 44 *)
Integrate[1/(c_^2 - x_^2), x_] /; FreeQ[c, x] := 1/(2 c) Log[(c + x)/(c - x)];

(* Formula 45 *)
Integrate[1/(x_^2 - c_^2), x_] /; FreeQ[c, x] := 1/(2 c) Log[(x - c)/(x + c)];

(* Formula 46: Split into + and - variations *)
Integrate[x_/(c_^2 + x_^2), x_] /; FreeQ[c, x] := 1/2 Log[c^2 + x^2];
Integrate[x_/(c_^2 - x_^2), x_] /; FreeQ[c, x] := -1/2 Log[c^2 - x^2];

(* Formula 47: Adapted pattern to match m instead of n+1 for better matching *)
Integrate[x_/(c_^2 + x_^2)^m_, x_] /; FreeQ[{c, m}, x] && m =!= 1 := -1/(2 (m - 1) (c^2 + x^2)^(m - 1));
Integrate[x_/(c_^2 - x_^2)^m_, x_] /; FreeQ[{c, m}, x] && m =!= 1 := 1/(2 (m - 1) (c^2 - x^2)^(m - 1));

(* Formula 48: Split into + and - variations *)
Integrate[1/(c_^2 + x_^2)^n_, x_] /; FreeQ[{c, n}, x] && n =!= 1 := 1/(2 c^2 (n - 1)) (x/(c^2 + x^2)^(n - 1) + (2 n - 3) Integrate[1/(c^2 + x^2)^(n - 1), x]);
Integrate[1/(c_^2 - x_^2)^n_, x_] /; FreeQ[{c, n}, x] && n =!= 1 := 1/(2 c^2 (n - 1)) (x/(c^2 - x^2)^(n - 1) + (2 n - 3) Integrate[1/(c^2 - x^2)^(n - 1), x]);

(* Formula 49 *)
Integrate[1/(x_^2 - c_^2)^n_, x_] /; FreeQ[{c, n}, x] && n =!= 1 := 1/(2 c^2 (n - 1)) (-x/(x^2 - c^2)^(n - 1) - (2 n - 3) Integrate[1/(x^2 - c^2)^(n - 1), x]);

(* Formula 50 *)
Integrate[x_/(x_^2 - c_^2), x_] /; FreeQ[c, x] := 1/2 Log[x^2 - c^2];

(* Formula 51: Adapted pattern to match m instead of n+1 *)
Integrate[x_/(x_^2 - c_^2)^m_, x_] /; FreeQ[{c, m}, x] && m =!= 1 := -1/(2 (m - 1) (x^2 - c^2)^(m - 1));

(* --- Section 5.4.4: Forms containing a + bx and c + dx --- *)

(* Formula 52 *)
Integrate[1/((a_ + b_. x_) (c_ + d_. x_)), x_] /; FreeQ[{a, b, c, d}, x] := With[{k = a d - b c, u = a + b x, v = c + d x}, 1/k Log[v/u]];

(* Formula 53 *)
Integrate[x_/((a_ + b_. x_) (c_ + d_. x_)), x_] /; FreeQ[{a, b, c, d}, x] := With[{k = a d - b c, u = a + b x, v = c + d x}, 1/k (a/b Log[u] - c/d Log[v])];

(* Formula 54 *)
Integrate[1/((a_ + b_. x_)^2 (c_ + d_. x_)), x_] /; FreeQ[{a, b, c, d}, x] := With[{k = a d - b c, u = a + b x, v = c + d x}, 1/k (1/u + d/k Log[v/u])];

(* Formula 55 *)
Integrate[x_/((a_ + b_. x_)^2 (c_ + d_. x_)), x_] /; FreeQ[{a, b, c, d}, x] := With[{k = a d - b c, u = a + b x, v = c + d x}, -(a/(b k u)) - c/k^2 Log[v/u]];

(* Formula 56 *)
Integrate[x_^2/((a_ + b_. x_)^2 (c_ + d_. x_)), x_] /; FreeQ[{a, b, c, d}, x] := With[{k = a d - b c, u = a + b x, v = c + d x}, a^2/(b^2 k u) + 1/k^2 (c^2/d Log[v] + (a (k - b c))/b^2 Log[u])];

(* Formula 57 *)
Integrate[1/((a_ + b_. x_)^n_ (c_ + d_. x_)^m_), x_] /; FreeQ[{a, b, c, d, n, m}, x] && m =!= 1 := With[{k = a d - b c, u = a + b x, v = c + d x}, 1/(k (m - 1)) (-1/(u^(n - 1) v^(m - 1)) - b (m + n - 2) Integrate[1/(u^n v^(m - 1)), x])];
  
(* Formula 58 *)
Integrate[(a_ + b_. x_)/(c_ + d_. x_), x_] /; FreeQ[{a, b, c, d}, x] := With[{k = a d - b c, u = a + b x, v = c + d x}, (b x)/d + k/d^2 Log[v]];

SetAttributes[Integrate, {Protected, ReadProtected}]
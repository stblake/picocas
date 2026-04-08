
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


(* Formula 59 *)
Integrate[(a_. + b_. x_)^m_. (c_. + d_. x_)^nn_., x_] /; FreeQ[{a, b, c, d, m, nn}, x] && IntegerQ[nn] && nn < -1 := With[{k = a d - b c, u = a + b x, v = c + d x, n = -nn}, -1/(k (n - 1)) (u^(m + 1)/v^(n - 1) + b (n - m - 2) Integrate[u^m/v^(n - 1), x])];

(* Formula 60 *)
Integrate[(a_. + b_. x_^2)^-1, x_] /; FreeQ[{a, b}, x] && a b > 0 := 1/Sqrt[a b] ArcTan[(x Sqrt[a b])/a];

(* Formula 61 *)
Integrate[(a_. + b_. x_^2)^-1, x_] /; FreeQ[{a, b}, x] && a b < 0 := 1/(2 Sqrt[-a b]) Log[(a + x Sqrt[-a b])/(a - x Sqrt[-a b])];

(* Formula 62 *)
Integrate[(a_^2 + b_^2 x_^2)^-1, x_] /; FreeQ[{a, b}, x] := 1/(a b) ArcTan[(b x)/a];

(* Formula 63 *)
Integrate[x_ (a_. + b_. x_^2)^-1, x_] /; FreeQ[{a, b}, x] := 1/(2 b) Log[a + b x^2];

(* Formula 64 *)
Integrate[x_^2 (a_. + b_. x_^2)^-1, x_] /; FreeQ[{a, b}, x] := x/b - a/b Integrate[(a + b x^2)^-1, x];

(* Formula 65 *)
Integrate[1/(a_ + b_. x_^2)^2, x_] /; FreeQ[{a, b}, x] := x/(2 a (a + b x^2)) + 1/(2 a) Integrate[1/(a + b x^2), x];

(* Formula 66 *)
Integrate[1/(a_^2 - b_^2 x_^2), x_] /; FreeQ[{a, b}, x] := 1/(2 a b) Log[(a + b x)/(a - b x)];

(* Formula 67: Mapped n+1 to m *)
Integrate[1/(a_ + b_. x_^2)^m_, x_] /; FreeQ[{a, b, m}, x] && m =!= 1 := x/(2 a (m - 1) (a + b x^2)^(m - 1)) + (2 m - 3)/(2 a (m - 1)) Integrate[1/(a + b x^2)^(m - 1), x];

(* Formula 68: Mapped m+1 to m *)
Integrate[x_/(a_ + b_. x_^2)^m_, x_] /; FreeQ[{a, b, m}, x] && m =!= 1 := -1/(2 b (m - 1) (a + b x^2)^(m - 1));

(* Formula 69: Mapped m+1 to m *)
Integrate[x_^2/(a_ + b_. x_^2)^m_, x_] /; FreeQ[{a, b, m}, x] && m =!= 1 := -x/(2 b (m - 1) (a + b x^2)^(m - 1)) + 1/(2 b (m - 1)) Integrate[1/(a + b x^2)^(m - 1), x];

(* Formula 70 *)
Integrate[1/(x_ (a_ + b_. x_^2)), x_] /; FreeQ[{a, b}, x] := 1/(2 a) Log[x^2/(a + b x^2)];

(* Formula 71 *)
Integrate[1/(x_^2 (a_ + b_. x_^2)), x_] /; FreeQ[{a, b}, x] := -1/(a x) - b/a Integrate[1/(a + b x^2), x];

(* Formula 72: Mapped n+1 to n *)
Integrate[1/(x_ (a_ + b_. x_^2)^n_), x_] /; FreeQ[{a, b, n}, x] && n =!= 1 := 1/(2 a (n - 1) (a + b x^2)^(n - 1)) + 1/a Integrate[1/(x (a + b x^2)^(n - 1)), x];

(* Formula 73: Mapped m+1 to n *)
Integrate[1/(x_^2 (a_ + b_. x_^2)^n_), x_] /; FreeQ[{a, b, n}, x] && n =!= 1 := 1/a Integrate[1/(x^2 (a + b x^2)^(n - 1)), x] - b/a Integrate[1/(a + b x^2)^n, x];


(* Formula 74 *)
Integrate[1/(a_ + b_. x_^3), x_] /; FreeQ[{a, b}, x] := With[{k = (a/b)^(1/3)}, 
    k/(3 a) (1/2 Log[(k + x)^3/(a + b x^3)] + Sqrt[3] ArcTan[(2 x - k)/(k Sqrt[3])])];

(* Formula 75 *)
Integrate[x_/(a_ + b_. x_^3), x_] /; FreeQ[{a, b}, x] := With[{k = (a/b)^(1/3)}, 
    1/(3 b k) (1/2 Log[(a + b x^3)/(k + x)^3] + Sqrt[3] ArcTan[(2 x - k)/(k Sqrt[3])])];

(* Formula 76 *)
Integrate[x_^2/(a_ + b_. x_^3), x_] /; FreeQ[{a, b}, x] := 1/(3 b) Log[a + b x^3];

(* Formula 77: Split into sign conditions *)
Integrate[1/(a_ + b_. x_^4), x_] /; FreeQ[{a, b}, x] && a b > 0 := With[{k = (a/(4 b))^(1/4)}, 
    k/(2 a) (1/2 Log[(x^2 + 2 k x + 2 k^2)/(x^2 - 2 k x + 2 k^2)] + ArcTan[(2 k x)/(2 k^2 - x^2)])];

Integrate[1/(a_ + b_. x_^4), x_] /; FreeQ[{a, b}, x] && a b < 0 := With[{k = (-a/b)^(1/4)}, 
    k/(2 a) (1/2 Log[(x + k)/(x - k)] + ArcTan[x/k])];

(* Formula 78 & 79 *)
Integrate[x_/(a_ + b_. x_^4), x_] /; FreeQ[{a, b}, x] && a b > 0 := With[{k = Sqrt[a/b]}, 1/(2 b k) ArcTan[x^2/k]];

Integrate[x_/(a_ + b_. x_^4), x_] /; FreeQ[{a, b}, x] && a b < 0 := With[{k = Sqrt[-a/b]}, 1/(4 b k) Log[(x^2 - k)/(x^2 + k)]];

(* Formula 80 & 81 *)
Integrate[x_^2/(a_ + b_. x_^4), x_] /; FreeQ[{a, b}, x] && a b > 0 := With[{k = (a/(4 b))^(1/4)}, 
    1/(4 b k) (1/2 Log[(x^2 - 2 k x + 2 k^2)/(x^2 + 2 k x + 2 k^2)] + ArcTan[(2 k x)/(2 k^2 - x^2)])];

Integrate[x_^2/(a_ + b_. x_^4), x_] /; FreeQ[{a, b}, x] && a b < 0 := With[{k = (-a/b)^(1/4)}, 
    1/(4 b k) (Log[(x - k)/(x + k)] + 2 ArcTan[x/k])];

(* Formula 82 *)
Integrate[x_^3/(a_ + b_. x_^4), x_] /; FreeQ[{a, b}, x] := 1/(4 b) Log[a + b x^4];

(* Formula 83 *)
Integrate[1/(x_ (a_ + b_. x_^n_)), x_] /; FreeQ[{a, b, n}, x] && n =!= 0 := 1/(a n) Log[x^n/(a + b x^n)];

(* Formula 84: Mapped m+1 to m *)
Integrate[1/(a_ + b_. x_^n_)^m_, x_] /; FreeQ[{a, b, n, m}, x] && m =!= 1 := 1/a Integrate[1/(a + b x^n)^(m - 1), x] - b/a Integrate[x^n/(a + b x^n)^m, x];

(* Formula 85: Mapped p+1 to p *)
Integrate[x_^m_/(a_ + b_. x_^n_)^p_, x_] /; FreeQ[{a, b, n, m, p}, x] && p =!= 1 := 1/b Integrate[x^(m - n)/(a + b x^n)^(p - 1), x] - a/b Integrate[x^(m - n)/(a + b x^n)^p, x];

(* Formula 86: Mapped p+1 to p *)
Integrate[1/(x_^m_ (a_ + b_. x_^n_)^p_), x_] /; FreeQ[{a, b, n, m, p}, x] && p =!= 1 := 1/a Integrate[1/(x^m (a + b x^n)^(p - 1)), x] - b/a Integrate[1/(x^(m - n) (a + b x^n)^p), x];

(* Formula 87: Form 1 *)
Integrate[x_^m_ (a_ + b_. x_^n_)^p_, x_] /; FreeQ[{a, b, n, m, p}, x] := 1/(b (n p + m + 1)) (x^(m - n + 1) (a + b x^n)^(p + 1) - a (m - n + 1) Integrate[x^(m - n) (a + b x^n)^p, x]);

(* Formula 88 *)
Integrate[1/(c_^3 + x_^3), x_] /; FreeQ[c, x] := 1/(6 c^2) Log[(c + x)^3/(c^3 + x^3)] + 1/(c^2 Sqrt[3]) ArcTan[(2 x - c)/(c Sqrt[3])];

Integrate[1/(c_^3 - x_^3), x_] /; FreeQ[c, x] := -1/(6 c^2) Log[(c - x)^3/(c^3 - x^3)] + 1/(c^2 Sqrt[3]) ArcTan[(2 x + c)/(c Sqrt[3])];

(* Formula 89 *)
Integrate[1/(c_^3 + x_^3)^2, x_] /; FreeQ[c, x] := x/(3 c^3 (c^3 + x^3)) + 2/(3 c^3) Integrate[1/(c^3 + x^3), x];

Integrate[1/(c_^3 - x_^3)^2, x_] /; FreeQ[c, x] := x/(3 c^3 (c^3 - x^3)) + 2/(3 c^3) Integrate[1/(c^3 - x^3), x];

(* Formula 90: Mapped n+1 to n *)
Integrate[1/(c_^3 + x_^3)^n_, x_] /; FreeQ[{c, n}, x] && n =!= 1 := 1/(3 (n - 1) c^3) (x/(c^3 + x^3)^(n - 1) + (3 n - 4) Integrate[1/(c^3 + x^3)^(n - 1), x]);

Integrate[1/(c_^3 - x_^3)^n_, x_] /; FreeQ[{c, n}, x] && n =!= 1 := 1/(3 (n - 1) c^3) (x/(c^3 - x^3)^(n - 1) + (3 n - 4) Integrate[1/(c^3 - x^3)^(n - 1), x]);

(* Formula 91 *)
Integrate[x_/(c_^3 + x_^3), x_] /; FreeQ[c, x] := 1/(6 c) Log[(c^3 + x^3)/(c + x)^3] + 1/(c Sqrt[3]) ArcTan[(2 x - c)/(c Sqrt[3])];

Integrate[x_/(c_^3 - x_^3), x_] /; FreeQ[c, x] := 1/(6 c) Log[(c^3 - x^3)/(c - x)^3] - 1/(c Sqrt[3]) ArcTan[(2 x + c)/(c Sqrt[3])];

(* Formula 92 *)
Integrate[x_/(c_^3 + x_^3)^2, x_] /; FreeQ[c, x] := x^2/(3 c^3 (c^3 + x^3)) + 1/(3 c^3) Integrate[x/(c^3 + x^3), x];

Integrate[x_/(c_^3 - x_^3)^2, x_] /; FreeQ[c, x] := x^2/(3 c^3 (c^3 - x^3)) + 1/(3 c^3) Integrate[x/(c^3 - x^3), x];

(* Formula 93: Mapped n+1 to n *)
Integrate[x_/(c_^3 + x_^3)^n_, x_] /; FreeQ[{c, n}, x] && n =!= 1 := 1/(3 (n - 1) c^3) (x^2/(c^3 + x^3)^(n - 1) + (3 n - 5) Integrate[x/(c^3 + x^3)^(n - 1), x]);

Integrate[x_/(c_^3 - x_^3)^n_, x_] /; FreeQ[{c, n}, x] && n =!= 1 := 1/(3 (n - 1) c^3) (x^2/(c^3 - x^3)^(n - 1) + (3 n - 5) Integrate[x/(c^3 - x^3)^(n - 1), x]);

(* Formula 94 *)
Integrate[x_^2/(c_^3 + x_^3), x_] /; FreeQ[c, x] := 1/3 Log[c^3 + x^3];

Integrate[x_^2/(c_^3 - x_^3), x_] /; FreeQ[c, x] := -1/3 Log[c^3 - x^3];

(* Formula 95: Mapped n+1 to n *)
Integrate[x_^2/(c_^3 + x_^3)^n_, x_] /; FreeQ[{c, n}, x] && n =!= 1 := -1/(3 (n - 1) (c^3 + x^3)^(n - 1));

Integrate[x_^2/(c_^3 - x_^3)^n_, x_] /; FreeQ[{c, n}, x] && n =!= 1 := 1/(3 (n - 1) (c^3 - x^3)^(n - 1));

(* Formula 96 *)
Integrate[1/(x_ (c_^3 + x_^3)), x_] /; FreeQ[c, x] := 1/(3 c^3) Log[x^3/(c^3 + x^3)];

Integrate[1/(x_ (c_^3 - x_^3)), x_] /; FreeQ[c, x] := 1/(3 c^3) Log[x^3/(c^3 - x^3)];

(* Formula 97 *)
Integrate[1/(x_ (c_^3 + x_^3)^2), x_] /; FreeQ[c, x] := 1/(3 c^3 (c^3 + x^3)) + 1/(3 c^6) Log[x^3/(c^3 + x^3)];

Integrate[1/(x_ (c_^3 - x_^3)^2), x_] /; FreeQ[c, x] := 1/(3 c^3 (c^3 - x^3)) + 1/(3 c^6) Log[x^3/(c^3 - x^3)];

(* Formula 98: Mapped n+1 to n *)
Integrate[1/(x_ (c_^3 + x_^3)^n_), x_] /; FreeQ[{c, n}, x] && n =!= 1 := 1/(3 (n - 1) c^3 (c^3 + x^3)^(n - 1)) + 1/c^3 Integrate[1/(x (c^3 + x^3)^(n - 1)), x];

Integrate[1/(x_ (c_^3 - x_^3)^n_), x_] /; FreeQ[{c, n}, x] && n =!= 1 := 1/(3 (n - 1) c^3 (c^3 - x^3)^(n - 1)) + 1/c^3 Integrate[1/(x (c^3 - x^3)^(n - 1)), x];

(* Formula 99 *)
Integrate[1/(x_^2 (c_^3 + x_^3)), x_] /; FreeQ[c, x] := -1/(c^3 x) - 1/c^3 Integrate[x/(c^3 + x^3), x];

Integrate[1/(x_^2 (c_^3 - x_^3)), x_] /; FreeQ[c, x] := -1/(c^3 x) + 1/c^3 Integrate[x/(c^3 - x^3), x];

(* Formula 100: Mapped n+1 to n *)
Integrate[1/(x_^2 (c_^3 + x_^3)^n_), x_] /; FreeQ[{c, n}, x] && n =!= 1 := 1/c^3 Integrate[1/(x^2 (c^3 + x^3)^(n - 1)), x] - 1/c^3 Integrate[x/(c^3 + x^3)^n, x];

Integrate[1/(x_^2 (c_^3 - x_^3)^n_), x_] /; FreeQ[{c, n}, x] && n =!= 1 := 1/c^3 Integrate[1/(x^2 (c^3 - x^3)^(n - 1)), x] + 1/c^3 Integrate[x/(c^3 - x^3)^n, x];

(* Formula 101 *)
Integrate[1/(c_^4 + x_^4), x_] /; FreeQ[c, x] := 1/(2 c^3 Sqrt[2]) (1/2 Log[(x^2 + c x Sqrt[2] + c^2)/(x^2 - c x Sqrt[2] + c^2)] + ArcTan[(c x Sqrt[2])/(c^2 - x^2)]);

(* Formula 102 *)
Integrate[1/(c_^4 - x_^4), x_] /; FreeQ[c, x] := 1/(2 c^3) (1/2 Log[(c + x)/(c - x)] + ArcTan[x/c]);

(* Formula 103 *)
Integrate[x_/(c_^4 + x_^4), x_] /; FreeQ[c, x] := 1/(2 c^2) ArcTan[x^2/c^2];

(* Formula 104 *)
Integrate[x_/(c_^4 - x_^4), x_] /; FreeQ[c, x] := 1/(4 c^2) Log[(c^2 + x^2)/(c^2 - x^2)];

(* Formula 105 *)
Integrate[x_^2/(c_^4 + x_^4), x_] /; FreeQ[c, x] := 1/(2 c Sqrt[2]) (1/2 Log[(x^2 - c x Sqrt[2] + c^2)/(x^2 + c x Sqrt[2] + c^2)] + ArcTan[(c x Sqrt[2])/(c^2 - x^2)]);

(* Formula 106 *)
Integrate[x_^2/(c_^4 - x_^4), x_] /; FreeQ[c, x] := 1/(2 c) (1/2 Log[(c + x)/(c - x)] - ArcTan[x/c]);

(* Formula 107 *)
Integrate[x_^3/(c_^4 + x_^4), x_] /; FreeQ[c, x] := 1/4 Log[c^4 + x^4];

Integrate[x_^3/(c_^4 - x_^4), x_] /; FreeQ[c, x] := -1/4 Log[c^4 - x^4];

(* Formula 108 *)
Integrate[1/(a_ + b_. x_ + c_. x_^2), x_] /; FreeQ[{a, b, c}, x] && 4 a c - b^2 > 0 := With[{q = 4 a c - b^2}, 2/Sqrt[q] ArcTan[(2 c x + b)/Sqrt[q]]];

Integrate[1/(a_ + b_. x_ + c_. x_^2), x_] /; FreeQ[{a, b, c}, x] && 4 a c - b^2 < 0 := With[{q = 4 a c - b^2}, 1/Sqrt[-q] Log[(2 c x + b - Sqrt[-q])/(2 c x + b + Sqrt[-q])]];

(* Formula 109 *)
Integrate[1/(a_ + b_. x_ + c_. x_^2)^2, x_] /; FreeQ[{a, b, c}, x] := With[{X = a + b x + c x^2, q = 4 a c - b^2}, 
    (2 c x + b)/(q X) + (2 c)/q Integrate[1/X, x]];

(* Formula 110 *)
Integrate[1/(a_ + b_. x_ + c_. x_^2)^3, x_] /; FreeQ[{a, b, c}, x] := With[{X = a + b x + c x^2, q = 4 a c - b^2}, 
    (2 c x + b)/q (1/(2 X^2) + (3 c)/(q X)) + (6 c^2)/q^2 Integrate[1/X, x]];

(* Formula 111: Mapped n+1 to n *)
Integrate[1/(a_ + b_. x_ + c_. x_^2)^n_, x_] /; FreeQ[{a, b, c, n}, x] && n =!= 1 := With[{X = a + b x + c x^2, q = 4 a c - b^2}, 
    (2 c x + b)/((n - 1) q X^(n - 1)) + (2 (2 n - 3) c)/(q (n - 1)) Integrate[1/X^(n - 1), x]];

(* Formula 112 *)
Integrate[x_/(a_ + b_. x_ + c_. x_^2), x_] /; FreeQ[{a, b, c}, x] := With[{X = a + b x + c x^2}, 
    1/(2 c) Log[X] - b/(2 c) Integrate[1/X, x]];

(* Formula 113 *)
Integrate[x_/(a_ + b_. x_ + c_. x_^2)^2, x_] /; FreeQ[{a, b, c}, x] := With[{X = a + b x + c x^2, q = 4 a c - b^2}, 
    -(b x + 2 a)/(q X) - b/q Integrate[1/X, x]];

(* Formula 114: Mapped n+1 to n *)
Integrate[x_/(a_ + b_. x_ + c_. x_^2)^n_, x_] /; FreeQ[{a, b, c, n}, x] && n =!= 1 := With[{X = a + b x + c x^2, q = 4 a c - b^2}, 
    -(2 a + b x)/((n - 1) q X^(n - 1)) - (b (2 n - 3))/((n - 1) q) Integrate[1/X^(n - 1), x]];

(* Formula 115 *)
Integrate[x_^2/(a_ + b_. x_ + c_. x_^2), x_] /; FreeQ[{a, b, c}, x] := With[{X = a + b x + c x^2}, 
    x/c - b/(2 c^2) Log[X] + (b^2 - 2 a c)/(2 c^2) Integrate[1/X, x]];

(* Formula 116 *)
Integrate[x_^2/(a_ + b_. x_ + c_. x_^2)^2, x_] /; FreeQ[{a, b, c}, x] := With[{X = a + b x + c x^2, q = 4 a c - b^2}, 
    ((b^2 - 2 a c) x + a b)/(c q X) + (2 a)/q Integrate[1/X, x]];

(* Formula 117: Mapped n+1 to n *)
Integrate[x_^m_/(a_ + b_. x_ + c_. x_^2)^n_, x_] /; FreeQ[{a, b, c, m, n}, x] && n =!= 1 := With[{X = a + b x + c x^2}, 
    -x^(m - 1)/((2 n - m - 1) c X^(n - 1)) - (n - m)*b/((2 n - m - 1) c) Integrate[x^(m - 1)/X^n, x] + (m - 1)*a/((2 n - m - 1) c) Integrate[x^(m - 2)/X^n, x]];

(* Formula 118 *)
Integrate[1/(x_ (a_ + b_. x_ + c_. x_^2)), x_] /; FreeQ[{a, b, c}, x] := With[{X = a + b x + c x^2}, 1/(2 a) Log[x^2/X] - b/(2 a) Integrate[1/X, x]];

(* Formula 119 *)
Integrate[1/(x_^2 (a_ + b_. x_ + c_. x_^2)), x_] /; FreeQ[{a, b, c}, x] := With[{X = a + b x + c x^2}, 
    b/(2 a^2) Log[X/x^2] - 1/(a x) + (b^2/(2 a^2) - c/a) Integrate[1/X, x]];

(* Formula 120: Mapped n to n-1 in denominators for smooth matching *)
Integrate[1/(x_ (a_ + b_. x_ + c_. x_^2)^n_), x_] /; FreeQ[{a, b, c, n}, x] && n =!= 1 := With[{X = a + b x + c x^2}, 
    1/(2 a (n - 1) X^(n - 1)) - b/(2 a) Integrate[1/X^n, x] + 1/a Integrate[1/(x X^(n - 1)), x]];

(* Formula 121: Mapped n+1 to n for smoother general matching *)
Integrate[1/(x_^m_ (a_ + b_. x_ + c_. x_^2)^n_), x_] /; FreeQ[{a, b, c, m, n}, x] && m =!= 1 := With[{X = a + b x + c x^2}, 
    -1/((m - 1) a x^(m - 1) X^(n - 1)) - ((n + m - 2) b)/((m - 1) a) Integrate[1/(x^(m - 1) X^n), x] - ((2 n + m - 3) c)/((m - 1) a) Integrate[1/(x^(m - 2) X^n), x]];

(* Formula 122 *)
Integrate[Sqrt[a_ + b_. x_], x_] /; FreeQ[{a, b}, x] := 2/(3 b) (a + b x)^(3/2);

(* Formula 123 *)
Integrate[x_ Sqrt[a_ + b_. x_], x_] /; FreeQ[{a, b}, x] := (-2 (2 a - 3 b x))/(15 b^2) (a + b x)^(3/2);

(* Formula 124 *)
Integrate[x_^2 Sqrt[a_ + b_. x_], x_] /; FreeQ[{a, b}, x] := (2 (8 a^2 - 12 a b x + 15 b^2 x^2))/(105 b^3) (a + b x)^(3/2);

(* Formula 125 *)
Integrate[x_^m_ Sqrt[a_ + b_. x_], x_] /; FreeQ[{a, b, m}, x] := 2/(b (2 m + 3)) (x^m (a + b x)^(3/2) - m a Integrate[x^(m - 1) Sqrt[a + b x], x]);

(* Formula 126 *)
Integrate[Sqrt[a_ + b_. x_]/x_, x_] /; FreeQ[{a, b}, x] := 2 Sqrt[a + b x] + a Integrate[1/(x Sqrt[a + b x]), x];

(* Formula 127 *)
Integrate[Sqrt[a_ + b_. x_]/x_^m_, x_] /; FreeQ[{a, b, m}, x] && m =!= 1 := -1/((m - 1) a) ((a + b x)^(3/2)/x^(m - 1) + ((2 m - 5) b)/2 Integrate[Sqrt[a + b x]/x^(m - 1), x]);

(* Formula 128 *)
Integrate[1/Sqrt[a_ + b_. x_], x_] /; FreeQ[{a, b}, x] := 2/b Sqrt[a + b x];

(* Formula 129 *)
Integrate[x_/Sqrt[a_ + b_. x_], x_] /; FreeQ[{a, b}, x] := (-2 (2 a - b x))/(3 b^2) Sqrt[a + b x];

(* Formula 130 *)
Integrate[x_^2/Sqrt[a_ + b_. x_], x_] /; FreeQ[{a, b}, x] := (2 (8 a^2 - 4 a b x + 3 b^2 x^2))/(15 b^3) Sqrt[a + b x];

(* Formula 131 *)
Integrate[x_^m_/Sqrt[a_ + b_. x_], x_] /; FreeQ[{a, b, m}, x] := 2/((2 m + 1) b) (x^m Sqrt[a + b x] - m a Integrate[x^(m - 1)/Sqrt[a + b x], x]);

(* Formula 132 *)
Integrate[1/(x_ Sqrt[a_ + b_. x_]), x_] /; FreeQ[{a, b}, x] && a < 0 := 2/Sqrt[-a] ArcTan[Sqrt[(a + b x)/-a]];

Integrate[1/(x_ Sqrt[a_ + b_. x_]), x_] /; FreeQ[{a, b}, x] && a > 0 := 1/Sqrt[a] Log[(Sqrt[a + b x] - Sqrt[a])/(Sqrt[a + b x] + Sqrt[a])];

(* Formula 133 *)
Integrate[1/(x_^2 Sqrt[a_ + b_. x_]), x_] /; FreeQ[{a, b}, x] := -Sqrt[a + b x]/(a x) - b/(2 a) Integrate[1/(x Sqrt[a + b x]), x];

(* Formula 134 *)
Integrate[1/(x_^n_ Sqrt[a_ + b_. x_]), x_] /; FreeQ[{a, b, n}, x] && n =!= 1 := -Sqrt[a + b x]/((n - 1) a x^(n - 1)) - ((2 n - 3) b)/(2 (n - 1) a) Integrate[1/(x^(n - 1) Sqrt[a + b x]), x];

(* Formula 135 & 136: Converted textbook ±n/2 to general rational power m_ *)
Integrate[(a_ + b_. x_)^m_, x_] /; FreeQ[{a, b, m}, x] && m =!= -1 := (a + b x)^(m + 1)/(b (m + 1));

Integrate[x_ (a_ + b_. x_)^m_, x_] /; FreeQ[{a, b, m}, x] && m =!= -1 && m =!= -2 := 1/b^2 ((a + b x)^(m + 2)/(m + 2) - a (a + b x)^(m + 1)/(m + 1));

(* Formula 137 & 138: Using general rational power m_ *)
Integrate[1/(x_ (a_ + b_. x_)^m_), x_] /; FreeQ[{a, b, m}, x] := 1/a Integrate[1/(x (a + b x)^(m - 1)), x] - b/a Integrate[1/(a + b x)^m, x];

Integrate[(a_ + b_. x_)^m_/x_, x_] /; FreeQ[{a, b, m}, x] := b Integrate[(a + b x)^(m - 1), x] + a Integrate[(a + b x)^(m - 1)/x, x];

(* Formula 139 *)
Integrate[1/Sqrt[(a_ + b_. x_) (c_ + d_. x_)], x_] /; FreeQ[{a, b, c, d}, x] && b d > 0 := 2/Sqrt[b d] ArcTanh[Sqrt[b d (a + b x) (c + d x)]/(b (c + d x))];

Integrate[1/Sqrt[(a_ + b_. x_) (c_ + d_. x_)], x_] /; FreeQ[{a, b, c, d}, x] && b d < 0 := 2/Sqrt[-b d] ArcTan[Sqrt[-b d (a + b x) (c + d x)]/(b (c + d x))];

(* Formula 140 *)
Integrate[Sqrt[(a_ + b_. x_) (c_ + d_. x_)], x_] /; FreeQ[{a, b, c, d}, x] := With[{u = a + b x, v = c + d x, k = a d - b c}, 
    (k + 2 b v)/(4 b d) Sqrt[u v] - k^2/(8 b d) Integrate[1/Sqrt[u v], x]];

(* Formula 141 *)
Integrate[1/((a_ + b_. x_) Sqrt[(a_ + b_. x_) (c_ + d_. x_)]), x_] /; FreeQ[{a, b, c, d}, x] && (a d - b c) d > 0 := With[{u = a + b x, v = c + d x, k = a d - b c}, 
    2/(k Sqrt[k d]) ArcTanh[Sqrt[k d u v]/(d u)]];

(* Formula 142 *)
Integrate[x_/Sqrt[(a_ + b_. x_) (c_ + d_. x_)], x_] /; FreeQ[{a, b, c, d}, x] := With[{u = a + b x, v = c + d x}, 
    Sqrt[u v]/(b d) - (a d + b c)/(2 b d) Integrate[1/Sqrt[u v], x]];

(* Formula 143 *)
Integrate[1/((c_ + d_. x_) Sqrt[(a_ + b_. x_) (c_ + d_. x_)]), x_] /; FreeQ[{a, b, c, d}, x] := With[{u = a + b x, v = c + d x, k = a d - b c}, 
    -(2 Sqrt[u v])/(k v)];

(* Formula 144 *)
Integrate[(c_ + d_. x_)/Sqrt[(a_ + b_. x_) (c_ + d_. x_)], x_] /; FreeQ[{a, b, c, d}, x] := With[{u = a + b x, v = c + d x, k = a d - b c}, 
    Sqrt[u v]/b - k/(2 b) Integrate[1/Sqrt[u v], x]];

(* Formula 145 *)
Integrate[Sqrt[(c_ + d_. x_)/(a_ + b_. x_)], x_] /; FreeQ[{a, b, c, d}, x] := With[{u = a + b x, v = c + d x}, 
    v/Abs[v] Integrate[v/Sqrt[u v], x]];

(* Formula 146 *)
Integrate[(c_ + d_. x_)^m_ Sqrt[a_ + b_. x_], x_] /; FreeQ[{a, b, c, d, m}, x] := With[{u = a + b x, v = c + d x, k = a d - b c}, 
    1/((2 m + 3) d) (2 v^(m + 1) Sqrt[u] + k Integrate[v^m/Sqrt[u], x])];

(* Formula 147 *)
Integrate[1/((c_ + d_. x_)^m_ Sqrt[a_ + b_. x_]), x_] /; FreeQ[{a, b, c, d, m}, x] && m =!= 1 := With[{u = a + b x, v = c + d x, k = a d - b c}, 
    -1/((m - 1) k) (Sqrt[u]/v^(m - 1) + (m - 3/2) b Integrate[1/(v^(m - 1) Sqrt[u]), x])];

(* Formula 148 *)
Integrate[(c_ + d_. x_)^m_/Sqrt[a_ + b_. x_], x_] /; FreeQ[{a, b, c, d, m}, x] := With[{u = a + b x, v = c + d x, k = a d - b c}, 
    2/(b (2 m + 1)) (v^m Sqrt[u] - m k Integrate[v^(m - 1)/Sqrt[u], x])];

(* Formula 149 *)
Integrate[Sqrt[x_^2 + a_^2], x_] /; FreeQ[a, x] := 1/2 (x Sqrt[x^2 + a^2] + a^2 Log[x + Sqrt[x^2 + a^2]]);

Integrate[Sqrt[x_^2 - a_^2], x_] /; FreeQ[a, x] := 1/2 (x Sqrt[x^2 - a^2] - a^2 Log[x + Sqrt[x^2 - a^2]]);

(* Formula 150 *)
Integrate[1/Sqrt[x_^2 + a_^2], x_] /; FreeQ[a, x] := Log[x + Sqrt[x^2 + a^2]];

Integrate[1/Sqrt[x_^2 - a_^2], x_] /; FreeQ[a, x] := Log[x + Sqrt[x^2 - a^2]];

(* Formula 151 *)
Integrate[1/(x_ Sqrt[x_^2 - a_^2]), x_] /; FreeQ[a, x] := 1/Abs[a] ArcSec[x/a];

(* Formula 152 *)
Integrate[1/(x_ Sqrt[x_^2 + a_^2]), x_] /; FreeQ[a, x] := -1/a Log[(a + Sqrt[x^2 + a^2])/x];

(* Formula 153 *)
Integrate[Sqrt[x_^2 + a_^2]/x_, x_] /; FreeQ[a, x] := Sqrt[x^2 + a^2] - a Log[(a + Sqrt[x^2 + a^2])/x];

(* Formula 154 *)
Integrate[Sqrt[x_^2 - a_^2]/x_, x_] /; FreeQ[a, x] := Sqrt[x^2 - a^2] - Abs[a] ArcSec[x/a];

(* Formula 155 *)
Integrate[x_/Sqrt[x_^2 + a_^2], x_] /; FreeQ[a, x] := Sqrt[x^2 + a^2];
Integrate[x_/Sqrt[x_^2 - a_^2], x_] /; FreeQ[a, x] := Sqrt[x^2 - a^2];

(* Formula 156 *)
Integrate[x_ Sqrt[x_^2 + a_^2], x_] /; FreeQ[a, x] := 1/3 (x^2 + a^2)^(3/2);
Integrate[x_ Sqrt[x_^2 - a_^2], x_] /; FreeQ[a, x] := 1/3 (x^2 - a^2)^(3/2);

(* Formula 157 *)
Integrate[(x_^2 + a_^2)^(3/2), x_] /; FreeQ[a, x] := 1/4 (x (x^2 + a^2)^(3/2) + (3 a^2 x)/2 Sqrt[x^2 + a^2] + (3 a^4)/2 Log[x + Sqrt[x^2 + a^2]]);

Integrate[(x_^2 - a_^2)^(3/2), x_] /; FreeQ[a, x] := 1/4 (x (x^2 - a^2)^(3/2) - (3 a^2 x)/2 Sqrt[x^2 - a^2] + (3 a^4)/2 Log[x + Sqrt[x^2 - a^2]]);

(* Formula 158 *)
Integrate[1/(x_^2 + a_^2)^(3/2), x_] /; FreeQ[a, x] := x/(a^2 Sqrt[x^2 + a^2]);
Integrate[1/(x_^2 - a_^2)^(3/2), x_] /; FreeQ[a, x] := -x/(a^2 Sqrt[x^2 - a^2]);

(* Formula 159 *)
Integrate[x_/(x_^2 + a_^2)^(3/2), x_] /; FreeQ[a, x] := -1/Sqrt[x^2 + a^2];
Integrate[x_/(x_^2 - a_^2)^(3/2), x_] /; FreeQ[a, x] := -1/Sqrt[x^2 - a^2];

(* Formula 160 *)
Integrate[x_ (x_^2 + a_^2)^(3/2), x_] /; FreeQ[a, x] := 1/5 (x^2 + a^2)^(5/2);
Integrate[x_ (x_^2 - a_^2)^(3/2), x_] /; FreeQ[a, x] := 1/5 (x^2 - a^2)^(5/2);

(* Formula 161 *)
Integrate[x_^2 Sqrt[x_^2 + a_^2], x_] /; FreeQ[a, x] := x/4 (x^2 + a^2)^(3/2) - (a^2 x)/8 Sqrt[x^2 + a^2] - a^4/8 Log[x + Sqrt[x^2 + a^2]];

Integrate[x_^2 Sqrt[x_^2 - a_^2], x_] /; FreeQ[a, x] := x/4 (x^2 - a^2)^(3/2) + (a^2 x)/8 Sqrt[x^2 - a^2] - a^4/8 Log[x + Sqrt[x^2 - a^2]];

(* Formula 162 *)
Integrate[x_^3 Sqrt[x_^2 + a_^2], x_] /; FreeQ[a, x] := 1/15 (3 x^2 - 2 a^2) (x^2 + a^2)^(3/2);

(* Formula 163 *)
Integrate[x_^3 Sqrt[x_^2 - a_^2], x_] /; FreeQ[a, x] := 1/5 (x^2 - a^2)^(5/2) + a^2/3 (x^2 - a^2)^(3/2);

(* Formula 164 *)
Integrate[x_^2/Sqrt[x_^2 + a_^2], x_] /; FreeQ[a, x] := x/2 Sqrt[x^2 + a^2] - a^2/2 Log[x + Sqrt[x^2 + a^2]];

Integrate[x_^2/Sqrt[x_^2 - a_^2], x_] /; FreeQ[a, x] := x/2 Sqrt[x^2 - a^2] + a^2/2 Log[x + Sqrt[x^2 - a^2]];

(* Formula 165 *)
Integrate[x_^3/Sqrt[x_^2 + a_^2], x_] /; FreeQ[a, x] := 1/3 (x^2 + a^2)^(3/2) - a^2 Sqrt[x^2 + a^2];
Integrate[x_^3/Sqrt[x_^2 - a_^2], x_] /; FreeQ[a, x] := 1/3 (x^2 - a^2)^(3/2) + a^2 Sqrt[x^2 - a^2];

(* Formula 166 *)
Integrate[1/(x_^2 Sqrt[x_^2 + a_^2]), x_] /; FreeQ[a, x] := -Sqrt[x^2 + a^2]/(a^2 x);
Integrate[1/(x_^2 Sqrt[x_^2 - a_^2]), x_] /; FreeQ[a, x] := Sqrt[x^2 - a^2]/(a^2 x);

(* Formula 167 *)
Integrate[1/(x_^3 Sqrt[x_^2 + a_^2]), x_] /; FreeQ[a, x] := -Sqrt[x^2 + a^2]/(2 a^2 x^2) + 1/(2 a^3) Log[(a + Sqrt[x^2 + a^2])/x];

(* Formula 168 *)
Integrate[1/(x_^3 Sqrt[x_^2 - a_^2]), x_] /; FreeQ[a, x] := Sqrt[x^2 - a^2]/(2 a^2 x^2) + 1/(2 Abs[a]^3) ArcSec[x/a];

(* Formula 169 *)
Integrate[x_^2 (x_^2 + a_^2)^(3/2), x_] /; FreeQ[a, x] := x/6 (x^2 + a^2)^(5/2) - (a^2 x)/24 (x^2 + a^2)^(3/2) - (a^4 x)/16 Sqrt[x^2 + a^2] - a^6/16 Log[x + Sqrt[x^2 + a^2]];

Integrate[x_^2 (x_^2 - a_^2)^(3/2), x_] /; FreeQ[a, x] := x/6 (x^2 - a^2)^(5/2) + (a^2 x)/24 (x^2 - a^2)^(3/2) - (a^4 x)/16 Sqrt[x^2 - a^2] + a^6/16 Log[x + Sqrt[x^2 - a^2]];

(* Formula 170 *)
Integrate[x_^3 (x_^2 + a_^2)^(3/2), x_] /; FreeQ[a, x] := 1/7 (x^2 + a^2)^(7/2) - a^2/5 (x^2 + a^2)^(5/2);
Integrate[x_^3 (x_^2 - a_^2)^(3/2), x_] /; FreeQ[a, x] := 1/7 (x^2 - a^2)^(7/2) + a^2/5 (x^2 - a^2)^(5/2);

(* Formula 171 *)
Integrate[Sqrt[x_^2 + a_^2]/x_^2, x_] /; FreeQ[a, x] := -Sqrt[x^2 + a^2]/x + Log[x + Sqrt[x^2 + a^2]];
Integrate[Sqrt[x_^2 - a_^2]/x_^2, x_] /; FreeQ[a, x] := -Sqrt[x^2 - a^2]/x + Log[x + Sqrt[x^2 - a^2]];

(* Formula 172 *)
Integrate[Sqrt[x_^2 + a_^2]/x_^3, x_] /; FreeQ[a, x] := -Sqrt[x^2 + a^2]/(2 x^2) - 1/(2 a) Log[(a + Sqrt[x^2 + a^2])/x];

(* Formula 173 *)
Integrate[Sqrt[x_^2 - a_^2]/x_^3, x_] /; FreeQ[a, x] := -Sqrt[x^2 - a^2]/(2 x^2) + 1/(2 Abs[a]) ArcSec[x/a];

(* Formula 174 *)
Integrate[Sqrt[x_^2 + a_^2]/x_^4, x_] /; FreeQ[a, x] := -(x^2 + a^2)^(3/2)/(3 a^2 x^3);
Integrate[Sqrt[x_^2 - a_^2]/x_^4, x_] /; FreeQ[a, x] := (x^2 - a^2)^(3/2)/(3 a^2 x^3);

(* Formula 175 *)
Integrate[x_^2/(x_^2 + a_^2)^(3/2), x_] /; FreeQ[a, x] := -x/Sqrt[x^2 + a^2] + Log[x + Sqrt[x^2 + a^2]];
Integrate[x_^2/(x_^2 - a_^2)^(3/2), x_] /; FreeQ[a, x] := -x/Sqrt[x^2 - a^2] + Log[x + Sqrt[x^2 - a^2]];

(* Formula 176 *)
Integrate[x_^3/(x_^2 + a_^2)^(3/2), x_] /; FreeQ[a, x] := Sqrt[x^2 + a^2] + a^2/Sqrt[x^2 + a^2];
Integrate[x_^3/(x_^2 - a_^2)^(3/2), x_] /; FreeQ[a, x] := Sqrt[x^2 - a^2] - a^2/Sqrt[x^2 - a^2];

(* Formula 177 *)
Integrate[1/(x_ (x_^2 + a_^2)^(3/2)), x_] /; FreeQ[a, x] := 1/(a^2 Sqrt[x^2 + a^2]) - 1/a^3 Log[(a + Sqrt[x^2 + a^2])/x];

(* Formula 178 *)
Integrate[1/(x_ (x_^2 - a_^2)^(3/2)), x_] /; FreeQ[a, x] := -1/(a^2 Sqrt[x^2 - a^2]) - 1/Abs[a]^3 ArcSec[x/a];

(* Formula 179 *)
Integrate[1/(x_^2 (x_^2 + a_^2)^(3/2)), x_] /; FreeQ[a, x] := -1/a^4 (Sqrt[x^2 + a^2]/x + x/Sqrt[x^2 + a^2]);
Integrate[1/(x_^2 (x_^2 - a_^2)^(3/2)), x_] /; FreeQ[a, x] := -1/a^4 (Sqrt[x^2 - a^2]/x + x/Sqrt[x^2 - a^2]);

(* Formula 180 *)
Integrate[1/(x_^3 (x_^2 + a_^2)^(3/2)), x_] /; FreeQ[a, x] := -1/(2 a^2 x^2 Sqrt[x^2 + a^2]) - 3/(2 a^4 Sqrt[x^2 + a^2]) + 3/(2 a^5) Log[(a + Sqrt[x^2 + a^2])/x];

(* Formula 181 *)
Integrate[1/(x_^3 (x_^2 - a_^2)^(3/2)), x_] /; FreeQ[a, x] := 
  Sqrt[x^2 - a^2]/(2 a^2 x^2) - 3/(2 a^4 Sqrt[x^2 - a^2]) - 3/(2 Abs[a]^5) ArcSec[x/a];

(* Formula 182: Recursive reductions for positive powers *)
Integrate[x_^m_/Sqrt[x_^2 + a_^2], x_] /; FreeQ[{a, m}, x] && m =!= 0 := x^(m - 1)/m Sqrt[x^2 + a^2] - ((m - 1) a^2)/m Integrate[x^(m - 2)/Sqrt[x^2 + a^2], x];

Integrate[x_^m_/Sqrt[x_^2 - a_^2], x_] /; FreeQ[{a, m}, x] && m =!= 0 := x^(m - 1)/m Sqrt[x^2 - a^2] + ((m - 1) a^2)/m Integrate[x^(m - 2)/Sqrt[x^2 - a^2], x];

(* Formula 185: Recursive reductions for negative powers *)
Integrate[1/(x_^m_ Sqrt[x_^2 + a_^2]), x_] /; FreeQ[{a, m}, x] && m =!= 1 := -Sqrt[x^2 + a^2]/((m - 1) a^2 x^(m - 1)) - (m - 2)/((m - 1) a^2) Integrate[1/(x^(m - 2) Sqrt[x^2 + a^2]), x];

Integrate[1/(x_^m_ Sqrt[x_^2 - a_^2]), x_] /; FreeQ[{a, m}, x] && m =!= 1 := Sqrt[x^2 - a^2]/((m - 1) a^2 x^(m - 1)) + (m - 2)/((m - 1) a^2) Integrate[1/(x^(m - 2) Sqrt[x^2 - a^2]), x];

(* Formula 189 & 190 *)
Integrate[1/((x_ - a_) Sqrt[x_^2 - a_^2]), x_] /; FreeQ[a, x] := -Sqrt[x^2 - a^2]/(a (x - a));
Integrate[1/((x_ + a_) Sqrt[x_^2 - a_^2]), x_] /; FreeQ[a, x] := Sqrt[x^2 - a^2]/(a (x + a));

(* Formula 191 *)
Integrate[Sqrt[a_^2 - x_^2], x_] /; FreeQ[a, x] := 
  1/2 (x Sqrt[a^2 - x^2] + a^2 ArcSin[x/Abs[a]]);

(* Formula 192 *)
Integrate[1/Sqrt[a_^2 - x_^2], x_] /; FreeQ[a, x] := ArcSin[x/Abs[a]];

(* Formula 193 *)
Integrate[1/(x_ Sqrt[a_^2 - x_^2]), x_] /; FreeQ[a, x] := -1/a Log[(a + Sqrt[a^2 - x^2])/x];

(* Formula 194 *)
Integrate[Sqrt[a_^2 - x_^2]/x_, x_] /; FreeQ[a, x] := Sqrt[a^2 - x^2] - a Log[(a + Sqrt[a^2 - x^2])/x];

(* Formula 195 & 196 *)
Integrate[x_/Sqrt[a_^2 - x_^2], x_] /; FreeQ[a, x] := -Sqrt[a^2 - x^2];
Integrate[x_ Sqrt[a_^2 - x_^2], x_] /; FreeQ[a, x] := -1/3 (a^2 - x^2)^(3/2);

(* Formula 197 *)
Integrate[(a_^2 - x_^2)^(3/2), x_] /; FreeQ[a, x] := 
  1/4 (x (a^2 - x^2)^(3/2) + (3 a^2 x)/2 Sqrt[a^2 - x^2] + (3 a^4)/2 ArcSin[x/Abs[a]]);

(* Formula 198 & 199 *)
Integrate[1/(a_^2 - x_^2)^(3/2), x_] /; FreeQ[a, x] := x/(a^2 Sqrt[a^2 - x^2]);
Integrate[x_/(a_^2 - x_^2)^(3/2), x_] /; FreeQ[a, x] := 1/Sqrt[a^2 - x^2];

(* Formula 200 *)
Integrate[x_ (a_^2 - x_^2)^(3/2), x_] /; FreeQ[a, x] := -1/5 (a^2 - x^2)^(5/2);

(* Formula 201 *)
Integrate[x_^2 Sqrt[a_^2 - x_^2], x_] /; FreeQ[a, x] := 
  -x/4 (a^2 - x^2)^(3/2) + a^2/8 (x Sqrt[a^2 - x^2] + a^2 ArcSin[x/Abs[a]]);

(* Formula 202 *)
Integrate[x_^3 Sqrt[a_^2 - x_^2], x_] /; FreeQ[a, x] := (-1/5 x^2 - 2/15 a^2) (a^2 - x^2)^(3/2);

(* Formula 203 *)
Integrate[x_^2 (a_^2 - x_^2)^(3/2), x_] /; FreeQ[a, x] := 
  -1/6 x (a^2 - x^2)^(5/2) + (a^2 x)/24 (a^2 - x^2)^(3/2) + (a^4 x)/16 Sqrt[a^2 - x^2] + a^6/16 ArcSin[x/Abs[a]];

(* Formula 204 *)
Integrate[x_^3 (a_^2 - x_^2)^(3/2), x_] /; FreeQ[a, x] := 1/7 (a^2 - x^2)^(7/2) - a^2/5 (a^2 - x^2)^(5/2);

(* Formula 205 *)
Integrate[x_^2/Sqrt[a_^2 - x_^2], x_] /; FreeQ[a, x] := 
  -x/2 Sqrt[a^2 - x^2] + a^2/2 ArcSin[x/Abs[a]];

(* Formula 206 *)
Integrate[1/(x_^2 Sqrt[a_^2 - x_^2]), x_] /; FreeQ[a, x] := -Sqrt[a^2 - x^2]/(a^2 x);

(* Formula 207 *)
Integrate[Sqrt[a_^2 - x_^2]/x_^2, x_] /; FreeQ[a, x] := -Sqrt[a^2 - x^2]/x - ArcSin[x/Abs[a]];

(* Formula 208 *)
Integrate[Sqrt[a_^2 - x_^2]/x_^3, x_] /; FreeQ[a, x] := 
  -Sqrt[a^2 - x^2]/(2 x^2) + 1/(2 a) Log[(a + Sqrt[a^2 - x^2])/x];

(* Formula 209 *)
Integrate[Sqrt[a_^2 - x_^2]/x_^4, x_] /; FreeQ[a, x] := -(a^2 - x^2)^(3/2)/(3 a^2 x^3);

(* Formula 210 *)
Integrate[x_^2/(a_^2 - x_^2)^(3/2), x_] /; FreeQ[a, x] := x/Sqrt[a^2 - x^2] - ArcSin[x/Abs[a]];

(* Formula 211 *)
Integrate[x_^3/Sqrt[a_^2 - x_^2], x_] /; FreeQ[a, x] := -2/3 (a^2 - x^2)^(3/2) - x^2 Sqrt[a^2 - x^2];

(* Formula 212 *)
Integrate[x_^3/(a_^2 - x_^2)^(3/2), x_] /; FreeQ[a, x] := 2 Sqrt[a^2 - x^2] + x^2/Sqrt[a^2 - x^2];

(* Formula 213 *)
Integrate[1/(x_^3 Sqrt[a_^2 - x_^2]), x_] /; FreeQ[a, x] := 
  -Sqrt[a^2 - x^2]/(2 a^2 x^2) - 1/(2 a^3) Log[(a + Sqrt[a^2 - x^2])/x];

(* Formula 214 *)
Integrate[1/(x_ (a_^2 - x_^2)^(3/2)), x_] /; FreeQ[a, x] := 
  1/(a^2 Sqrt[a^2 - x^2]) - 1/a^3 Log[(a + Sqrt[a^2 - x^2])/x];

(* Formula 215 *)
Integrate[1/(x_^2 (a_^2 - x_^2)^(3/2)), x_] /; FreeQ[a, x] := 
  1/a^4 (-Sqrt[a^2 - x^2]/x + x/Sqrt[a^2 - x^2]);

(* Formula 216 *)
Integrate[1/(x_^3 (a_^2 - x_^2)^(3/2)), x_] /; FreeQ[a, x] := 
  (3 x^2 - 2 a^2)/(2 a^4 Sqrt[a^2 - x^2]) - 3/(2 a^5) Log[(a + Sqrt[a^2 - x^2])/x];

(* Formula 217 & 220: General Reduction Rules *)
Integrate[x_^m_/Sqrt[a_^2 - x_^2], x_] /; FreeQ[{a, m}, x] && m =!= 0 := 
  -x^(m - 1)/m Sqrt[a^2 - x^2] + ((m - 1) a^2)/m Integrate[x^(m - 2)/Sqrt[a^2 - x^2], x];
Integrate[1/(x_^m_ Sqrt[a_^2 - x_^2]), x_] /; FreeQ[{a, m}, x] && m =!= 1 := 
  -Sqrt[a^2 - x^2]/((m - 1) a^2 x^(m - 1)) + (m - 2)/((m - 1) a^2) Integrate[1/(x^(m - 2) Sqrt[a^2 - x^2]), x];

(* Formula 223 *)
Integrate[1/((b_^2 - x_^2) Sqrt[a_^2 - x_^2]), x_] /; FreeQ[{a, b}, x] && a^2 > b^2 := 
  1/(2 b Sqrt[a^2 - b^2]) Log[(b Sqrt[a^2 - x^2] + x Sqrt[a^2 - b^2])/(b Sqrt[a^2 - x^2] - x Sqrt[a^2 - b^2])];
Integrate[1/((b_^2 - x_^2) Sqrt[a_^2 - x_^2]), x_] /; FreeQ[{a, b}, x] && b^2 > a^2 := 
  1/(b Sqrt[b^2 - a^2]) ArcTan[(x Sqrt[b^2 - a^2])/(b Sqrt[a^2 - x^2])];

(* Formula 224 *)
Integrate[1/((b_^2 + x_^2) Sqrt[a_^2 - x_^2]), x_] /; FreeQ[{a, b}, x] := 
  1/(b Sqrt[a^2 + b^2]) ArcTan[(x Sqrt[a^2 + b^2])/(b Sqrt[a^2 - x^2])];

(* Formula 225 *)
Integrate[Sqrt[a_^2 - x_^2]/(b_^2 + x_^2), x_] /; FreeQ[{a, b}, x] := 
  Sqrt[a^2 + b^2]/Abs[b] ArcSin[(x Sqrt[a^2 + b^2])/(Abs[a] Sqrt[x^2 + b^2])] - ArcSin[x/Abs[a]];

(* Formula 226 *)
Integrate[1/Sqrt[a_ + b_. x_ + c_. x_^2], x_] /; FreeQ[{a, b, c}, x] && c > 0 := 
  1/Sqrt[c] Log[2 Sqrt[c (a + b x + c x^2)] + 2 c x + b];
Integrate[1/Sqrt[a_ + b_. x_ + c_. x_^2], x_] /; FreeQ[{a, b, c}, x] && c < 0 := 
  -1/Sqrt[-c] ArcSin[(2 c x + b)/Sqrt[b^2 - 4 a c]];

(* Formula 227 & 228 *)
Integrate[1/(a_ + b_. x_ + c_. x_^2)^(3/2), x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2, q = 4 a c - b^2}, (2 (2 c x + b))/(q Sqrt[X])];
Integrate[1/(a_ + b_. x_ + c_. x_^2)^(5/2), x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2, q = 4 a c - b^2, k = (4 c)/q}, 
    (2 (2 c x + b))/(3 q Sqrt[X]) (1/X + 2 k)
  ];

(* Formula 229: Generic reduction for n (mapped from texts n) *)
Integrate[1/((a_ + b_. x_ + c_. x_^2)^n_ Sqrt[a_ + b_. x_ + c_. x_^2]), x_] /; FreeQ[{a, b, c, n}, x] && n =!= 0 := 
  With[{X = a + b x + c x^2, q = 4 a c - b^2, k = (4 c)/q}, 
    (2 (2 c x + b) Sqrt[X])/((2 n + 1) q X^(n + 1)) + (2 k n)/(2 n + 1) Integrate[1/(X^n Sqrt[X]), x]
  ];

(* Formula 230 - 232 *)
Integrate[Sqrt[a_ + b_. x_ + c_. x_^2], x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2, q = 4 a c - b^2, k = (4 c)/q}, 
    ((2 c x + b) Sqrt[X])/(4 c) + 1/(2 k) Integrate[1/Sqrt[X], x]
  ];
Integrate[(a_ + b_. x_ + c_. x_^2)^(3/2), x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2, q = 4 a c - b^2, k = (4 c)/q}, 
    ((2 c x + b) Sqrt[X])/(8 c) (X + 3/(2 k)) + 3/(8 k^2) Integrate[1/Sqrt[X], x]
  ];
Integrate[(a_ + b_. x_ + c_. x_^2)^(5/2), x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2, q = 4 a c - b^2, k = (4 c)/q}, 
    ((2 c x + b) Sqrt[X])/(12 c) (X^2 + (5 X)/(4 k) + 15/(8 k^2)) + 5/(16 k^3) Integrate[1/Sqrt[X], x]
  ];

(* Formula 233: General reduction *)
Integrate[(a_ + b_. x_ + c_. x_^2)^n_ Sqrt[a_ + b_. x_ + c_. x_^2], x_] /; FreeQ[{a, b, c, n}, x] := 
  With[{X = a + b x + c x^2, q = 4 a c - b^2, k = (4 c)/q}, 
    ((2 c x + b) X^n Sqrt[X])/(4 (n + 1) c) + (2 n + 1)/(2 (n + 1) k) Integrate[X^(n - 1) Sqrt[X], x]
  ];

(* Formula 234 *)
Integrate[x_/Sqrt[a_ + b_. x_ + c_. x_^2], x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2}, 
    Sqrt[X]/c - b/(2 c) Integrate[1/Sqrt[X], x]
  ];

(* Formula 235 & 236 *)
Integrate[x_/(a_ + b_. x_ + c_. x_^2)^(3/2), x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2, q = 4 a c - b^2}, -(2 (b x + 2 a))/(q Sqrt[X])];
Integrate[x_/((a_ + b_. x_ + c_. x_^2)^n_ Sqrt[a_ + b_. x_ + c_. x_^2]), x_] /; FreeQ[{a, b, c, n}, x] && n =!= 0 := 
  With[{X = a + b x + c x^2}, 
    -Sqrt[X]/((2 n - 1) c X^n) - b/(2 c) Integrate[1/(X^n Sqrt[X]), x]
  ];

(* Formula 237 - 239 *)
Integrate[x_^2/Sqrt[a_ + b_. x_ + c_. x_^2], x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2}, 
    (x/(2 c) - (3 b)/(4 c^2)) Sqrt[X] + (3 b^2 - 4 a c)/(8 c^2) Integrate[1/Sqrt[X], x]
  ];
Integrate[x_^2/(a_ + b_. x_ + c_. x_^2)^(3/2), x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2, q = 4 a c - b^2}, 
    ((2 b^2 - 4 a c) x + 2 a b)/(c q Sqrt[X]) + 1/c Integrate[1/Sqrt[X], x]
  ];
Integrate[x_^2/((a_ + b_. x_ + c_. x_^2)^n_ Sqrt[a_ + b_. x_ + c_. x_^2]), x_] /; FreeQ[{a, b, c, n}, x] && n =!= 0 := 
  With[{X = a + b x + c x^2, q = 4 a c - b^2}, 
    ((2 b^2 - 4 a c) x + 2 a b)/((2 n - 1) c q X^(n - 1) Sqrt[X]) + (4 a c + (2 n - 3) b^2)/((2 n - 1) c q) Integrate[1/(X^(n - 1) Sqrt[X]), x]
  ];

(* Formula 240 & 241 *)
Integrate[x_^3/Sqrt[a_ + b_. x_ + c_. x_^2], x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2}, 
    (x^2/(3 c) - (5 b x)/(12 c^2) + (5 b^2)/(8 c^3) - (2 a)/(3 c^2)) Sqrt[X] + ((3 a b)/(4 c^2) - (5 b^3)/(16 c^3)) Integrate[1/Sqrt[X], x]
  ];
Integrate[x_^n_/Sqrt[a_ + b_. x_ + c_. x_^2], x_] /; FreeQ[{a, b, c, n}, x] && n =!= 0 := 
  With[{X = a + b x + c x^2}, 
    1/(n c) x^(n - 1) Sqrt[X] - ((2 n - 1) b)/(2 n c) Integrate[x^(n - 1)/Sqrt[X], x] - ((n - 1) a)/(n c) Integrate[x^(n - 2)/Sqrt[X], x]
  ];

(* Formula 242 - 245 *)
Integrate[x_ (a_ + b_. x_ + c_. x_^2)^(3/2), x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2, q = 4 a c - b^2, k = (4 c)/q}, 
    (X Sqrt[X])/(3 c) - (b (2 c x + b))/(8 c^2) Sqrt[X] - b/(4 c k) Integrate[1/Sqrt[X], x]
  ];
Integrate[x_ X_^n_ Sqrt[X_], x_] /; FreeQ[{a, b, c, n}, x] := 
  With[{X = a + b x + c x^2}, 
    (X^(n + 1) Sqrt[X])/((2 n + 3) c) - b/(2 c) Integrate[X^n Sqrt[X], x]
  ];
Integrate[x_^2 Sqrt[a_ + b_. x_ + c_. x_^2], x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2}, 
    (x - (5 b)/(6 c)) (X Sqrt[X])/(4 c) + (5 b^2 - 4 a c)/(16 c^2) Integrate[Sqrt[X], x]
  ];

(* Formula 246 *)
Integrate[1/(x_ Sqrt[a_ + b_. x_ + c_. x_^2]), x_] /; FreeQ[{a, b, c}, x] && a > 0 := 
  With[{X = a + b x + c x^2}, -1/Sqrt[a] Log[(2 Sqrt[a X] + 2 a + b x)/x]];
Integrate[1/(x_ Sqrt[a_ + b_. x_ + c_. x_^2]), x_] /; FreeQ[{a, b, c}, x] && a < 0 := 
  1/Sqrt[-a] ArcSin[(b x + 2 a)/(x Sqrt[b^2 - 4 a c])];

(* Formula 247 - 249 *)
Integrate[1/(x_^2 Sqrt[a_ + b_. x_ + c_. x_^2]), x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2}, -Sqrt[X]/(a x) - b/(2 a) Integrate[1/(x Sqrt[X]), x]];
Integrate[Sqrt[a_ + b_. x_ + c_. x_^2]/x_, x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2}, Sqrt[X] + b/2 Integrate[1/Sqrt[X], x] + a Integrate[1/(x Sqrt[X]), x]];
Integrate[Sqrt[a_ + b_. x_ + c_. x_^2]/x_^2, x_] /; FreeQ[{a, b, c}, x] := 
  With[{X = a + b x + c x^2}, -Sqrt[X]/x + b/2 Integrate[1/(x Sqrt[X]), x] + c Integrate[1/Sqrt[X], x]];

(* Formula 250 *)
Integrate[Sqrt[2 a_ x_ - x_^2], x_] /; FreeQ[a, x] := 
  1/2 ((x - a) Sqrt[2 a x - x^2] + a^2 ArcSin[(x - a)/Abs[a]]);

(* Formula 251 *)
Integrate[1/Sqrt[2 a_ x_ - x_^2], x_] /; FreeQ[a, x] := 
  ArcSin[(x - a)/Abs[a]];

(* Formula 252 *)
Integrate[x_^n_ Sqrt[2 a_ x_ - x_^2], x_] /; FreeQ[{a, n}, x] && n =!= -2 := 
  -((x^(n - 1) (2 a x - x^2)^(3/2))/(n + 2)) + ((2 n + 1) a)/(n + 2) Integrate[x^(n - 1) Sqrt[2 a x - x^2], x];

(* Formula 253 *)
Integrate[Sqrt[2 a_ x_ - x_^2]/x_^n_, x_] /; FreeQ[{a, n}, x] && n =!= 3/2 := 
  (2 a x - x^2)^(3/2)/((3 - 2 n) a x^n) + (n - 3)/((2 n - 3) a) Integrate[Sqrt[2 a x - x^2]/x^(n - 1), x];

(* Formula 254 *)
Integrate[x_^n_/Sqrt[2 a_ x_ - x_^2], x_] /; FreeQ[{a, n}, x] && n =!= 0 := 
  -(x^(n - 1) Sqrt[2 a x - x^2])/n + (a (2 n - 1))/n Integrate[x^(n - 1)/Sqrt[2 a x - x^2], x];

(* Formula 255 *)
Integrate[1/(x_^n_ Sqrt[2 a_ x_ - x_^2]), x_] /; FreeQ[{a, n}, x] && n =!= 1/2 := 
  Sqrt[2 a x - x^2]/(a (1 - 2 n) x^n) + (n - 1)/((2 n - 1) a) Integrate[1/(x^(n - 1) Sqrt[2 a x - x^2]), x];

(* Formula 256 *)
Integrate[1/(2 a_ x_ - x_^2)^(3/2), x_] /; FreeQ[a, x] := 
  (x - a)/(a^2 Sqrt[2 a x - x^2]);

(* Formula 257 *)
Integrate[x_/(2 a_ x_ - x_^2)^(3/2), x_] /; FreeQ[a, x] := 
  x/(a Sqrt[2 a x - x^2]);

(* Formula 258 *)
Integrate[1/Sqrt[2 a_ x_ + x_^2], x_] /; FreeQ[a, x] := 
  Log[x + a + Sqrt[2 a x + x^2]];

(* Formula 259 *)
Integrate[Sqrt[a_. x_^2 + c_], x_] /; FreeQ[{a, c}, x] && a < 0 := 
  x/2 Sqrt[a x^2 + c] + c/(2 Sqrt[-a]) ArcSin[x Sqrt[-a/c]];
Integrate[Sqrt[a_. x_^2 + c_], x_] /; FreeQ[{a, c}, x] && a > 0 := 
  x/2 Sqrt[a x^2 + c] + c/(2 Sqrt[a]) Log[x Sqrt[a] + Sqrt[a x^2 + c]];

(* Formula 260 *)
Integrate[Sqrt[(1 + x_)/(1 - x_)], x_] := 
  ArcSin[x] - Sqrt[1 - x^2];

(* Formula 261 *)
Integrate[1/(x_ Sqrt[a_. x_^n_ + c_]), x_] /; FreeQ[{a, c, n}, x] && c > 0 := 
  1/(n Sqrt[c]) Log[(Sqrt[a x^n + c] - Sqrt[c])/(Sqrt[a x^n + c] + Sqrt[c])];
Integrate[1/(x_ Sqrt[a_. x_^n_ + c_]), x_] /; FreeQ[{a, c, n}, x] && c < 0 := 
  2/(n Sqrt[-c]) ArcSec[Sqrt[-(a x^n)/c]];

(* Formula 262 *)
Integrate[1/Sqrt[a_. x_^2 + c_], x_] /; FreeQ[{a, c}, x] && a < 0 := 
  1/Sqrt[-a] ArcSin[x Sqrt[-a/c]];
Integrate[1/Sqrt[a_. x_^2 + c_], x_] /; FreeQ[{a, c}, x] && a > 0 := 
  1/Sqrt[a] Log[x Sqrt[a] + Sqrt[a x^2 + c]];

(* Formula 263: Generalized m+1/2 exponent to general m_ *)
Integrate[(a_. x_^2 + c_)^m_, x_] /; FreeQ[{a, c, m}, x] && m =!= -1/2 := 
  (x (a x^2 + c)^m)/(2 m + 1) + (2 m c)/(2 m + 1) Integrate[(a x^2 + c)^(m - 1), x];

(* Formula 264 *)
Integrate[x_ (a_. x_^2 + c_)^m_, x_] /; FreeQ[{a, c, m}, x] && m =!= -1 := 
  (a x^2 + c)^(m + 1)/(2 a (m + 1));

(* Formula 265 *)
Integrate[(a_. x_^2 + c_)^m_/x_, x_] /; FreeQ[{a, c, m}, x] && m =!= 0 := 
  (a x^2 + c)^m/(2 m) + c Integrate[(a x^2 + c)^(m - 1)/x, x];

(* Formula 266 *)
Integrate[1/(a_. x_^2 + c_)^m_, x_] /; FreeQ[{a, c, m}, x] && m =!= 1 := 
  x/((2 m - 2) c (a x^2 + c)^(m - 1)) + (2 m - 3)/((2 m - 2) c) Integrate[1/(a x^2 + c)^(m - 1), x];

(* Formula 267: Already covered structurally by 261 out of the box! *)

(* Formula 268 *)
Integrate[(1 + x_^2)/((1 - x_^2) Sqrt[1 + x_^4]), x_] := 
  1/Sqrt[2] Log[(x Sqrt[2] + Sqrt[1 + x^4])/(1 - x^2)];

(* Formula 269 *)
Integrate[(1 - x_^2)/((1 + x_^2) Sqrt[1 + x_^4]), x_] := 
  1/Sqrt[2] ArcTan[(x Sqrt[2])/Sqrt[1 + x^4]];

(* Formula 270 *)
Integrate[1/(x_ Sqrt[x_^n_ + a_^2]), x_] /; FreeQ[{a, n}, x] := 
  -2/(n a) Log[(a + Sqrt[x^n + a^2])/Sqrt[x^n]];

(* Formula 271 *)
Integrate[1/(x_ Sqrt[x_^n_ - a_^2]), x_] /; FreeQ[{a, n}, x] := 
  -2/(n a) ArcSin[a/Sqrt[x^n]];

(* Formula 272 *)
Integrate[Sqrt[x_/(a_^3 - x_^3)], x_] /; FreeQ[a, x] := 
  2/3 ArcSin[(x/a)^(3/2)];

(* Formula 273 *)
Integrate[Sin[a_. x_], x_] /; FreeQ[a, x] := -Cos[a x]/a;

(* Formula 274 *)
Integrate[Cos[a_. x_], x_] /; FreeQ[a, x] := Sin[a x]/a;

(* Formula 275 *)
Integrate[Tan[a_. x_], x_] /; FreeQ[a, x] := -Log[Cos[a x]]/a;

(* Formula 276 *)
Integrate[Cot[a_. x_], x_] /; FreeQ[a, x] := Log[Sin[a x]]/a;

(* Formula 277 *)
Integrate[Sec[a_. x_], x_] /; FreeQ[a, x] := Log[Sec[a x] + Tan[a x]]/a;

(* Formula 278 *)
Integrate[Csc[a_. x_], x_] /; FreeQ[a, x] := Log[Csc[a x] - Cot[a x]]/a;

(* Formula 279 *)
Integrate[Sin[a_. x_]^2, x_] /; FreeQ[a, x] := x/2 - Sin[2 a x]/(4 a);

(* Formula 280 *)
Integrate[Sin[a_. x_]^3, x_] /; FreeQ[a, x] := -1/(3 a) Cos[a x] (Sin[a x]^2 + 2);

(* Formula 281 *)
Integrate[Sin[a_. x_]^4, x_] /; FreeQ[a, x] := (3 x)/8 - Sin[2 a x]/(4 a) + Sin[4 a x]/(32 a);

(* Formula 282 *)
Integrate[Sin[a_. x_]^n_, x_] /; FreeQ[{a, n}, x] && n =!= 0 := 
  -(Sin[a x]^(n - 1) Cos[a x])/(n a) + (n - 1)/n Integrate[Sin[a x]^(n - 2), x];

(* Formula 285 *)
Integrate[Cos[a_. x_]^2, x_] /; FreeQ[a, x] := x/2 + Sin[2 a x]/(4 a);

(* Formula 286 *)
Integrate[Cos[a_. x_]^3, x_] /; FreeQ[a, x] := 1/(3 a) Sin[a x] (Cos[a x]^2 + 2);

(* Formula 287 *)
Integrate[Cos[a_. x_]^4, x_] /; FreeQ[a, x] := (3 x)/8 + Sin[2 a x]/(4 a) + Sin[4 a x]/(32 a);

(* Formula 288 *)
Integrate[Cos[a_. x_]^n_, x_] /; FreeQ[{a, n}, x] && n =!= 0 := 
  (Cos[a x]^(n - 1) Sin[a x])/(n a) + (n - 1)/n Integrate[Cos[a x]^(n - 2), x];

(* Formula 291 *)
Integrate[1/Sin[a_. x_]^2, x_] /; FreeQ[a, x] := -Cot[a x]/a;

(* Formula 292 *)
Integrate[1/Sin[a_. x_]^m_, x_] /; FreeQ[{a, m}, x] && m =!= 1 := 
  -Cos[a x]/(a (m - 1) Sin[a x]^(m - 1)) + (m - 2)/(m - 1) Integrate[1/Sin[a x]^(m - 2), x];

(* Formula 295 *)
Integrate[1/Cos[a_. x_]^2, x_] /; FreeQ[a, x] := Tan[a x]/a;

(* Formula 296 *)
Integrate[1/Cos[a_. x_]^m_, x_] /; FreeQ[{a, m}, x] && m =!= 1 := 
  Sin[a x]/(a (m - 1) Cos[a x]^(m - 1)) + (m - 2)/(m - 1) Integrate[1/Cos[a x]^(m - 2), x];

(* Formula 299 *)
Integrate[Sin[m_. x_] Sin[n_. x_], x_] /; FreeQ[{m, n}, x] && m^2 =!= n^2 := 
  Sin[(m - n) x]/(2 (m - n)) - Sin[(m + n) x]/(2 (m + n));

(* Formula 300 *)
Integrate[Cos[m_. x_] Cos[n_. x_], x_] /; FreeQ[{m, n}, x] && m^2 =!= n^2 := 
  Sin[(m - n) x]/(2 (m - n)) + Sin[(m + n) x]/(2 (m + n));

(* Formula 301 *)
Integrate[Sin[a_. x_] Cos[a_. x_], x_] /; FreeQ[a, x] := Sin[a x]^2/(2 a);

(* Formula 302 *)
Integrate[Sin[m_. x_] Cos[n_. x_], x_] /; FreeQ[{m, n}, x] && m^2 =!= n^2 := 
  -Cos[(m - n) x]/(2 (m - n)) - Cos[(m + n) x]/(2 (m + n));

(* Formula 303 *)
Integrate[Sin[a_. x_]^2 Cos[a_. x_]^2, x_] /; FreeQ[a, x] := x/8 - Sin[4 a x]/(32 a);

(* Formula 304 *)
Integrate[Sin[a_. x_] Cos[a_. x_]^m_, x_] /; FreeQ[{a, m}, x] && m =!= -1 := 
  -Cos[a x]^(m + 1)/((m + 1) a);

(* Formula 305 *)
Integrate[Sin[a_. x_]^m_ Cos[a_. x_], x_] /; FreeQ[{a, m}, x] && m =!= -1 := 
  Sin[a x]^(m + 1)/((m + 1) a);

(* Formula 306 *)
Integrate[Cos[a_. x_]^m_ Sin[a_. x_]^n_, x_] /; FreeQ[{a, m, n}, x] && m + n =!= 0 := 
  -(Cos[a x]^(m - 1) Sin[a x]^(n + 1))/((m + n) a) + (m - 1)/(m + n) Integrate[Cos[a x]^(m - 2) Sin[a x]^n, x];

(* Formula 307 *)
Integrate[Cos[a_. x_]^m_/Sin[a_. x_]^n_, x_] /; FreeQ[{a, m, n}, x] && n =!= 1 := 
  -Cos[a x]^(m + 1)/(a (n - 1) Sin[a x]^(n - 1)) - (m - n + 2)/(n - 1) Integrate[Cos[a x]^m/Sin[a x]^(n - 2), x];

(* Formula 308 *)
Integrate[Sin[a_. x_]^m_/Cos[a_. x_]^n_, x_] /; FreeQ[{a, m, n}, x] && n =!= 1 := 
  Sin[a x]^(m - 1)/(a (n - 1) Cos[a x]^(n - 1)) - (m - n + 2)/(n - 1) Integrate[Sin[a x]^m/Cos[a x]^(n - 2), x];

(* Formula 309 *)
Integrate[Sin[a_. x_]/Cos[a_. x_]^2, x_] /; FreeQ[a, x] := Sec[a x]/a;

(* Formula 310 *)
Integrate[Sin[a_. x_]^2/Cos[a_. x_], x_] /; FreeQ[a, x] := 
  -Sin[a x]/a + 1/a Log[Tan[Pi/4 + (a x)/2]];

(* Formula 311 *)
Integrate[Cos[a_. x_]/Sin[a_. x_]^2, x_] /; FreeQ[a, x] := -Csc[a x]/a;

(* Formula 312 *)
Integrate[1/(Sin[a_. x_] Cos[a_. x_]), x_] /; FreeQ[a, x] := Log[Tan[a x]]/a;

(* Formula 313 *)
Integrate[1/(Sin[a_. x_] Cos[a_. x_]^2), x_] /; FreeQ[a, x] := 
  1/a (Sec[a x] + Log[Tan[(a x)/2]]);

(* Formula 314 *)
Integrate[1/(Sin[a_. x_] Cos[a_. x_]^n_), x_] /; FreeQ[{a, n}, x] && n =!= 1 := 
  1/(a (n - 1) Cos[a x]^(n - 1)) + Integrate[1/(Sin[a x] Cos[a x]^(n - 2)), x];

(* Formula 315 *)
Integrate[1/(Sin[a_. x_]^2 Cos[a_. x_]), x_] /; FreeQ[a, x] := 
  -Csc[a x]/a + 1/a Log[Tan[Pi/4 + (a x)/2]];

(* Formula 316 *)
Integrate[1/(Sin[a_. x_]^2 Cos[a_. x_]^2), x_] /; FreeQ[a, x] := -2/a Cot[2 a x];

(* Formula 317 *)
Integrate[1/(Sin[a_. x_]^m_ Cos[a_. x_]^n_), x_] /; FreeQ[{a, m, n}, x] && m =!= 1 := 
  -(1/(a (m - 1) Sin[a x]^(m - 1) Cos[a x]^(n - 1))) + (m + n - 2)/(m - 1) Integrate[1/(Sin[a x]^(m - 2) Cos[a x]^n), x];

(* Formula 318 *)
Integrate[Sin[a_ + b_. x_], x_] /; FreeQ[{a, b}, x] := -Cos[a + b x]/b;

(* Formula 319 *)
Integrate[Cos[a_ + b_. x_], x_] /; FreeQ[{a, b}, x] := Sin[a + b x]/b;

(* Formula 320 *)
Integrate[1/(1 + Sin[a_. x_]), x_] /; FreeQ[a, x] := -1/a Tan[Pi/4 - (a x)/2];
Integrate[1/(1 - Sin[a_. x_]), x_] /; FreeQ[a, x] := 1/a Tan[Pi/4 + (a x)/2];

(* Formula 321 *)
Integrate[1/(1 + Cos[a_. x_]), x_] /; FreeQ[a, x] := 
  1/a Tan[(a x)/2];

(* Formula 322 *)
Integrate[1/(1 - Cos[a_. x_]), x_] /; FreeQ[a, x] := 
  -1/a Cot[(a x)/2];

(* Formula 323 *)
Integrate[1/(a_ + b_. Sin[x_]), x_] /; FreeQ[{a, b}, x] && a^2 > b^2 := 
  2/Sqrt[a^2 - b^2] ArcTan[(a Tan[x/2] + b)/Sqrt[a^2 - b^2]];
Integrate[1/(a_ + b_. Sin[x_]), x_] /; FreeQ[{a, b}, x] && b^2 > a^2 := 
  1/Sqrt[b^2 - a^2] Log[(a Tan[x/2] + b - Sqrt[b^2 - a^2])/(a Tan[x/2] + b + Sqrt[b^2 - a^2])];

(* Formula 324 *)
Integrate[1/(a_ + b_. Cos[x_]), x_] /; FreeQ[{a, b}, x] && a^2 > b^2 := 
  2/Sqrt[a^2 - b^2] ArcTan[(Sqrt[a^2 - b^2] Tan[x/2])/(a + b)];
Integrate[1/(a_ + b_. Cos[x_]), x_] /; FreeQ[{a, b}, x] && b^2 > a^2 := 
  1/Sqrt[b^2 - a^2] Log[(Sqrt[b^2 - a^2] Tan[x/2] + a + b)/(Sqrt[b^2 - a^2] Tan[x/2] - a - b)];

(* Formula 327 *)
Integrate[1/(a_^2 Cos[x_]^2 + b_^2 Sin[x_]^2), x_] /; FreeQ[{a, b}, x] := 
  1/(a b) ArcTan[(b Tan[x])/a];

(* Formula 329 *)
Integrate[(Sin[c_. x_] Cos[c_. x_])/(a_ Cos[c_. x_]^2 + b_ Sin[c_. x_]^2), x_] /; FreeQ[{a, b, c}, x] && a =!= b := 
  1/(2 c (b - a)) Log[a Cos[c x]^2 + b Sin[c x]^2];

(* Formula 330 *)
Integrate[1/(a_ + b_. Tan[c_. x_]), x_] /; FreeQ[{a, b, c}, x] := 
  1/(c (a^2 + b^2)) (a c x + b Log[a Cos[c x] + b Sin[c x]]);

(* Formula 331 *)
Integrate[1/(b_ + a_. Cot[c_. x_]), x_] /; FreeQ[{a, b, c}, x] := 
  1/(c (a^2 + b^2)) (b c x - a Log[a Cos[c x] + b Sin[c x]]);

(* Formula 333 *)
Integrate[Sin[a_. x_]/(1 + Sin[a_. x_]), x_] /; FreeQ[a, x] := 
  x + 1/a Tan[Pi/4 - (a x)/2];
Integrate[Sin[a_. x_]/(1 - Sin[a_. x_]), x_] /; FreeQ[a, x] := 
  -x + 1/a Tan[Pi/4 + (a x)/2];

(* Formula 334 *)
Integrate[1/(Sin[a_. x_] (1 + Sin[a_. x_])), x_] /; FreeQ[a, x] := 
  1/a Tan[Pi/4 - (a x)/2] + 1/a Log[Tan[(a x)/2]];
Integrate[1/(Sin[a_. x_] (1 - Sin[a_. x_])), x_] /; FreeQ[a, x] := 
  1/a Tan[Pi/4 + (a x)/2] + 1/a Log[Tan[(a x)/2]];

(* Formula 335 & 336 *)
Integrate[1/(1 + Sin[a_. x_])^2, x_] /; FreeQ[a, x] := 
  -1/(2 a) Tan[Pi/4 - (a x)/2] - 1/(6 a) Tan[Pi/4 - (a x)/2]^3;
Integrate[1/(1 - Sin[a_. x_])^2, x_] /; FreeQ[a, x] := 
  1/(2 a) Cot[Pi/4 - (a x)/2] + 1/(6 a) Cot[Pi/4 - (a x)/2]^3;

(* Formula 337 & 338 *)
Integrate[Sin[a_. x_]/(1 + Sin[a_. x_])^2, x_] /; FreeQ[a, x] := 
  -1/(2 a) Tan[Pi/4 - (a x)/2] + 1/(6 a) Tan[Pi/4 - (a x)/2]^3;
Integrate[Sin[a_. x_]/(1 - Sin[a_. x_])^2, x_] /; FreeQ[a, x] := 
  -1/(2 a) Cot[Pi/4 - (a x)/2] + 1/(6 a) Cot[Pi/4 - (a x)/2]^3;

(* Formula 339 *)
Integrate[Sin[x_]/(a_ + b_. Sin[x_]), x_] /; FreeQ[{a, b}, x] := 
  x/b - a/b Integrate[1/(a + b Sin[x]), x];

(* Formula 340 *)
Integrate[1/(Sin[x_] (a_ + b_. Sin[x_])), x_] /; FreeQ[{a, b}, x] := 
  1/a Log[Tan[x/2]] - b/a Integrate[1/(a + b Sin[x]), x];

(* Formula 341 *)
Integrate[1/(a_ + b_. Sin[x_])^2, x_] /; FreeQ[{a, b}, x] && a^2 =!= b^2 := 
  (b Cos[x])/((a^2 - b^2) (a + b Sin[x])) + a/(a^2 - b^2) Integrate[1/(a + b Sin[x]), x];

(* Formula 342 & 343 *)
Integrate[1/(a_^2 + b_^2 Sin[c_. x_]^2), x_] /; FreeQ[{a, b, c}, x] := 
  1/(a c Sqrt[a^2 + b^2]) ArcTan[(Sqrt[a^2 + b^2] Tan[c x])/a];
Integrate[1/(a_^2 - b_^2 Sin[c_. x_]^2), x_] /; FreeQ[{a, b, c}, x] && a^2 > b^2 := 
  1/(a c Sqrt[a^2 - b^2]) ArcTan[(Sqrt[a^2 - b^2] Tan[c x])/a];
Integrate[1/(a_^2 - b_^2 Sin[c_. x_]^2), x_] /; FreeQ[{a, b, c}, x] && b^2 > a^2 := 
  1/(2 a c Sqrt[b^2 - a^2]) Log[(Sqrt[b^2 - a^2] Tan[c x] + a)/(Sqrt[b^2 - a^2] Tan[c x] - a)];

(* Formula 344 & 345 *)
Integrate[Cos[a_. x_]/(1 + Cos[a_. x_]), x_] /; FreeQ[a, x] := 
  x - 1/a Tan[(a x)/2];
Integrate[Cos[a_. x_]/(1 - Cos[a_. x_]), x_] /; FreeQ[a, x] := 
  -x - 1/a Cot[(a x)/2];

(* Formula 346 & 347 *)
Integrate[1/(Cos[a_. x_] (1 + Cos[a_. x_])), x_] /; FreeQ[a, x] := 
  1/a Log[Tan[Pi/4 + (a x)/2]] - 1/a Tan[(a x)/2];
Integrate[1/(Cos[a_. x_] (1 - Cos[a_. x_])), x_] /; FreeQ[a, x] := 
  1/a Log[Tan[Pi/4 + (a x)/2]] - 1/a Cot[(a x)/2];

(* Formula 348 & 349 *)
Integrate[1/(1 + Cos[a_. x_])^2, x_] /; FreeQ[a, x] := 
  1/(2 a) Tan[(a x)/2] + 1/(6 a) Tan[(a x)/2]^3;
Integrate[1/(1 - Cos[a_. x_])^2, x_] /; FreeQ[a, x] := 
  -1/(2 a) Cot[(a x)/2] - 1/(6 a) Cot[(a x)/2]^3;

(* Formula 350 & 351 *)
Integrate[Cos[a_. x_]/(1 + Cos[a_. x_])^2, x_] /; FreeQ[a, x] := 
  1/(2 a) Tan[(a x)/2] - 1/(6 a) Tan[(a x)/2]^3;
Integrate[Cos[a_. x_]/(1 - Cos[a_. x_])^2, x_] /; FreeQ[a, x] := 
  1/(2 a) Cot[(a x)/2] - 1/(6 a) Cot[(a x)/2]^3;

(* Formula 352 *)
Integrate[Cos[x_]/(a_ + b_. Cos[x_]), x_] /; FreeQ[{a, b}, x] := 
  x/b - a/b Integrate[1/(a + b Cos[x]), x];

(* Formula 353 *)
Integrate[1/(Cos[x_] (a_ + b_. Cos[x_])), x_] /; FreeQ[{a, b}, x] := 
  1/a Log[Tan[x/2 + Pi/4]] - b/a Integrate[1/(a + b Cos[x]), x];

(* Formula 354 *)
Integrate[1/(a_ + b_. Cos[x_])^2, x_] /; FreeQ[{a, b}, x] && a^2 =!= b^2 := 
  (b Sin[x])/((b^2 - a^2) (a + b Cos[x])) - a/(b^2 - a^2) Integrate[1/(a + b Cos[x]), x];

(* Formula 355 *)
Integrate[Cos[x_]/(a_ + b_. Cos[x_])^2, x_] /; FreeQ[{a, b}, x] && a^2 =!= b^2 := 
  (a Sin[x])/((a^2 - b^2) (a + b Cos[x])) - b/(a^2 - b^2) Integrate[1/(a + b Cos[x]), x];

(* Formula 356 *)
Integrate[1/(a_^2 + b_^2 - 2 a_ b_ Cos[c_. x_]), x_] /; FreeQ[{a, b, c}, x] := 
  2/(c (a^2 - b^2)) ArcTan[((a + b) Tan[(c x)/2])/(a - b)];

(* Formula 357 & 358 *)
Integrate[1/(a_^2 + b_^2 Cos[c_. x_]^2), x_] /; FreeQ[{a, b, c}, x] := 
  1/(a c Sqrt[a^2 + b^2]) ArcTan[(a Tan[c x])/Sqrt[a^2 + b^2]];
Integrate[1/(a_^2 - b_^2 Cos[c_. x_]^2), x_] /; FreeQ[{a, b, c}, x] && a^2 > b^2 := 
  1/(a c Sqrt[a^2 - b^2]) ArcTan[(a Tan[c x])/Sqrt[a^2 - b^2]];
Integrate[1/(a_^2 - b_^2 Cos[c_. x_]^2), x_] /; FreeQ[{a, b, c}, x] && b^2 > a^2 := 
  1/(2 a c Sqrt[b^2 - a^2]) Log[(a Tan[c x] - Sqrt[b^2 - a^2])/(a Tan[c x] + Sqrt[b^2 - a^2])];

(* Formula 359 *)
Integrate[Sin[a_. x_]/(1 + Cos[a_. x_]), x_] /; FreeQ[a, x] := -1/a Log[1 + Cos[a x]];
Integrate[Sin[a_. x_]/(1 - Cos[a_. x_]), x_] /; FreeQ[a, x] := 1/a Log[1 - Cos[a x]];

(* Formula 360 *)
Integrate[Cos[a_. x_]/(1 + Sin[a_. x_]), x_] /; FreeQ[a, x] := 1/a Log[1 + Sin[a x]];
Integrate[Cos[a_. x_]/(1 - Sin[a_. x_]), x_] /; FreeQ[a, x] := -1/a Log[1 - Sin[a x]];

(* Formula 361 *)
Integrate[1/(Sin[a_. x_] (1 + Cos[a_. x_])), x_] /; FreeQ[a, x] := 
  1/(2 a (1 + Cos[a x])) + 1/(2 a) Log[Tan[(a x)/2]];
Integrate[1/(Sin[a_. x_] (1 - Cos[a_. x_])), x_] /; FreeQ[a, x] := 
  -1/(2 a (1 - Cos[a x])) + 1/(2 a) Log[Tan[(a x)/2]];

(* Formula 362 *)
Integrate[1/(Cos[a_. x_] (1 + Sin[a_. x_])), x_] /; FreeQ[a, x] := 
  -1/(2 a (1 + Sin[a x])) + 1/(2 a) Log[Tan[(a x)/2 + Pi/4]];
Integrate[1/(Cos[a_. x_] (1 - Sin[a_. x_])), x_] /; FreeQ[a, x] := 
  1/(2 a (1 - Sin[a x])) + 1/(2 a) Log[Tan[(a x)/2 + Pi/4]];

(* Formula 363 *)
Integrate[Sin[a_. x_]/(Cos[a_. x_] (1 + Cos[a_. x_])), x_] /; FreeQ[a, x] := 1/a Log[Sec[a x] + 1];
Integrate[Sin[a_. x_]/(Cos[a_. x_] (1 - Cos[a_. x_])), x_] /; FreeQ[a, x] := 1/a Log[Sec[a x] - 1];

(* Formula 364 *)
Integrate[Cos[a_. x_]/(Sin[a_. x_] (1 + Sin[a_. x_])), x_] /; FreeQ[a, x] := -1/a Log[Csc[a x] + 1];
Integrate[Cos[a_. x_]/(Sin[a_. x_] (1 - Sin[a_. x_])), x_] /; FreeQ[a, x] := -1/a Log[Csc[a x] - 1];

(* Formula 365 *)
Integrate[Sin[a_. x_]/(Cos[a_. x_] (1 + Sin[a_. x_])), x_] /; FreeQ[a, x] := 
  1/(2 a (1 + Sin[a x])) + 1/(2 a) Log[Tan[(a x)/2 + Pi/4]];
Integrate[Sin[a_. x_]/(Cos[a_. x_] (1 - Sin[a_. x_])), x_] /; FreeQ[a, x] := 
  1/(2 a (1 - Sin[a x])) - 1/(2 a) Log[Tan[(a x)/2 + Pi/4]];

(* Formula 366 *)
Integrate[Cos[a_. x_]/(Sin[a_. x_] (1 + Cos[a_. x_])), x_] /; FreeQ[a, x] := 
  -1/(2 a (1 + Cos[a x])) + 1/(2 a) Log[Tan[(a x)/2]];
Integrate[Cos[a_. x_]/(Sin[a_. x_] (1 - Cos[a_. x_])), x_] /; FreeQ[a, x] := 
  -1/(2 a (1 - Cos[a x])) - 1/(2 a) Log[Tan[(a x)/2]];

(* Formula 367 *)
Integrate[1/(Sin[a_. x_] + Cos[a_. x_]), x_] /; FreeQ[a, x] := 
  1/(a Sqrt[2]) Log[Tan[(a x)/2 + Pi/8]];
Integrate[1/(Sin[a_. x_] - Cos[a_. x_]), x_] /; FreeQ[a, x] := 
  1/(a Sqrt[2]) Log[Tan[(a x)/2 - Pi/8]];

(* Formula 368 *)
Integrate[1/(Sin[a_. x_] + Cos[a_. x_])^2, x_] /; FreeQ[a, x] := 
  1/(2 a) Tan[a x - Pi/4];
Integrate[1/(Sin[a_. x_] - Cos[a_. x_])^2, x_] /; FreeQ[a, x] := 
  1/(2 a) Tan[a x + Pi/4];

(* Formula 369 *)
Integrate[1/(1 + Cos[a_. x_] + Sin[a_. x_]), x_] /; FreeQ[a, x] := 
  1/a Log[1 + Tan[(a x)/2]];
Integrate[1/(1 + Cos[a_. x_] - Sin[a_. x_]), x_] /; FreeQ[a, x] := 
  -1/a Log[1 - Tan[(a x)/2]];

(* Formula 370 *)
Integrate[1/(a_^2 Cos[c_. x_]^2 - b_^2 Sin[c_. x_]^2), x_] /; FreeQ[{a, b, c}, x] := 
  1/(2 a b c) Log[(b Tan[c x] + a)/(b Tan[c x] - a)];

(* Formula 371 *)
Integrate[x_ Sin[a_. x_], x_] /; FreeQ[a, x] := 
  1/a^2 Sin[a x] - x/a Cos[a x];

(* Formula 372 *)
Integrate[x_^2 Sin[a_. x_], x_] /; FreeQ[a, x] := 
  (2 x)/a^2 Sin[a x] + (2 - a^2 x^2)/a^3 Cos[a x];

(* Formula 373 *)
Integrate[x_^3 Sin[a_. x_], x_] /; FreeQ[a, x] := 
  (3 a^2 x^2 - 6)/a^4 Sin[a x] + (6 x - a^2 x^3)/a^3 Cos[a x];

(* Formula 374: General Reduction *)
Integrate[x_^m_ Sin[a_. x_], x_] /; FreeQ[{a, m}, x] && m > 0 := 
  -(x^m Cos[a x]/a) + m/a Integrate[x^(m - 1) Cos[a x], x];

(* Formula 375 *)
Integrate[x_ Cos[a_. x_], x_] /; FreeQ[a, x] := 
  1/a^2 Cos[a x] + x/a Sin[a x];

(* Formula 376 *)
Integrate[x_^2 Cos[a_. x_], x_] /; FreeQ[a, x] := 
  (2 x)/a^2 Cos[a x] + (a^2 x^2 - 2)/a^3 Sin[a x];

(* Formula 377 *)
Integrate[x_^3 Cos[a_. x_], x_] /; FreeQ[a, x] := 
  (3 a^2 x^2 - 6)/a^4 Cos[a x] + (a^2 x^3 - 6 x)/a^3 Sin[a x];

(* Formula 378: General Reduction *)
Integrate[x_^m_ Cos[a_. x_], x_] /; FreeQ[{a, m}, x] && m > 0 := 
  x^m Sin[a x]/a - m/a Integrate[x^(m - 1) Sin[a x], x];

(* Formula 381 *)
Integrate[x_ Sin[a_. x_]^2, x_] /; FreeQ[a, x] := 
  x^2/4 - x/(4 a) Sin[2 a x] - 1/(8 a^2) Cos[2 a x];

(* Formula 382 *)
Integrate[x_^2 Sin[a_. x_]^2, x_] /; FreeQ[a, x] := 
  x^3/6 - (x^2/(4 a) - 1/(8 a^3)) Sin[2 a x] - x/(4 a^2) Cos[2 a x];

(* Formula 383 *)
Integrate[x_ Sin[a_. x_]^3, x_] /; FreeQ[a, x] := 
  x/(12 a) Cos[3 a x] - 1/(36 a^2) Sin[3 a x] - (3 x)/(4 a) Cos[a x] + 3/(4 a^2) Sin[a x];

(* Formula 384 *)
Integrate[x_ Cos[a_. x_]^2, x_] /; FreeQ[a, x] := 
  x^2/4 + x/(4 a) Sin[2 a x] + 1/(8 a^2) Cos[2 a x];

(* Formula 385 *)
Integrate[x_^2 Cos[a_. x_]^2, x_] /; FreeQ[a, x] := 
  x^3/6 + (x^2/(4 a) - 1/(8 a^3)) Sin[2 a x] + x/(4 a^2) Cos[2 a x];

(* Formula 386 *)
Integrate[x_ Cos[a_. x_]^3, x_] /; FreeQ[a, x] := 
  x/(12 a) Sin[3 a x] + 1/(36 a^2) Cos[3 a x] + (3 x)/(4 a) Sin[a x] + 3/(4 a^2) Cos[a x];

(* Formula 387: Reduction for Negative Powers *)
Integrate[Sin[a_. x_]/x_^m_, x_] /; FreeQ[{a, m}, x] && m =!= 1 := 
  -Sin[a x]/((m - 1) x^(m - 1)) + a/(m - 1) Integrate[Cos[a x]/x^(m - 1), x];

(* Formula 388: Reduction for Negative Powers *)
Integrate[Cos[a_. x_]/x_^m_, x_] /; FreeQ[{a, m}, x] && m =!= 1 := 
  -Cos[a x]/((m - 1) x^(m - 1)) - a/(m - 1) Integrate[Sin[a x]/x^(m - 1), x];

(* Formula 389 *)
Integrate[x_/(1 + Sin[a_. x_]), x_] /; FreeQ[a, x] := 
  -(x Cos[a x])/(a (1 + Sin[a x])) + 1/a^2 Log[1 + Sin[a x]];
Integrate[x_/(1 - Sin[a_. x_]), x_] /; FreeQ[a, x] := 
  (x Cos[a x])/(a (1 - Sin[a x])) + 1/a^2 Log[1 - Sin[a x]];

(* Formula 390 *)
Integrate[x_/(1 + Cos[a_. x_]), x_] /; FreeQ[a, x] := 
  x/a Tan[(a x)/2] + 2/a^2 Log[Cos[(a x)/2]];

(* Formula 391 *)
Integrate[x_/(1 - Cos[a_. x_]), x_] /; FreeQ[a, x] := 
  -x/a Cot[(a x)/2] + 2/a^2 Log[Sin[(a x)/2]];

(* Formula 392 *)
Integrate[(x_ + Sin[x_])/(1 + Cos[x_]), x_] := x Tan[x/2];

(* Formula 393 *)
Integrate[(x_ - Sin[x_])/(1 - Cos[x_]), x_] := -x Cot[x/2];

(* Formula 394 *)
Integrate[Sqrt[1 - Cos[a_. x_]], x_] /; FreeQ[a, x] := -(2 Sqrt[2])/a Cos[(a x)/2];

(* Formula 395 *)
Integrate[Sqrt[1 + Cos[a_. x_]], x_] /; FreeQ[a, x] := (2 Sqrt[2])/a Sin[(a x)/2];

(* Formula 396: Primary Branch *)
Integrate[Sqrt[1 + Sin[x_]], x_] := 2 (Sin[x/2] - Cos[x/2]);

(* Formula 397: Primary Branch *)
Integrate[Sqrt[1 - Sin[x_]], x_] := 2 (Sin[x/2] + Cos[x/2]);

(* Formula 398 *)
Integrate[1/Sqrt[1 - Cos[x_]], x_] := Sqrt[2] Log[Tan[x/4]];

(* Formula 399 *)
Integrate[1/Sqrt[1 + Cos[x_]], x_] := Sqrt[2] Log[Tan[(x + Pi)/4]];

(* Formula 400 *)
Integrate[1/Sqrt[1 - Sin[x_]], x_] := Sqrt[2] Log[Tan[x/4 - Pi/8]];

(* Formula 401: Primary Branch *)
Integrate[1/Sqrt[1 + Sin[x_]], x_] := 
  Sqrt[2] Log[Tan[x/4 + Pi/8]];

(* Formula 402 *)
Integrate[Tan[a_. x_]^2, x_] /; FreeQ[a, x] := 
  Tan[a x]/a - x;

(* Formula 403 *)
Integrate[Tan[a_. x_]^3, x_] /; FreeQ[a, x] := 
  Tan[a x]^2/(2 a) + 1/a Log[Cos[a x]];

(* Formula 404 *)
Integrate[Tan[a_. x_]^4, x_] /; FreeQ[a, x] := 
  Tan[a x]^3/(3 a) - Tan[a x]/a + x;

(* Formula 405 *)
Integrate[Tan[a_. x_]^n_, x_] /; FreeQ[{a, n}, x] && n =!= 1 := 
  Tan[a x]^(n - 1)/(a (n - 1)) - Integrate[Tan[a x]^(n - 2), x];

(* Formula 406 *)
Integrate[Cot[a_. x_]^2, x_] /; FreeQ[a, x] := 
  -Cot[a x]/a - x;

(* Formula 407 *)
Integrate[Cot[a_. x_]^3, x_] /; FreeQ[a, x] := 
  -Cot[a x]^2/(2 a) - 1/a Log[Sin[a x]];

(* Formula 408 *)
Integrate[Cot[a_. x_]^4, x_] /; FreeQ[a, x] := 
  -Cot[a x]^3/(3 a) + Cot[a x]/a + x;

(* Formula 409 *)
Integrate[Cot[a_. x_]^n_, x_] /; FreeQ[{a, n}, x] && n =!= 1 := 
  -Cot[a x]^(n - 1)/(a (n - 1)) - Integrate[Cot[a x]^(n - 2), x];

(* Formula 410 *)
Integrate[x_/Sin[a_. x_]^2, x_] /; FreeQ[a, x] := 
  -(x Cot[a x])/a + 1/a^2 Log[Sin[a x]];

(* Formula 411 *)
Integrate[x_/Sin[a_. x_]^n_, x_] /; FreeQ[{a, n}, x] && n =!= 1 && n =!= 2 := 
  -(x Cos[a x])/(a (n - 1) Sin[a x]^(n - 1)) - 1/(a^2 (n - 1) (n - 2) Sin[a x]^(n - 2)) + (n - 2)/(n - 1) Integrate[x/Sin[a x]^(n - 2), x];

(* Formula 412 *)
Integrate[x_/Cos[a_. x_]^2, x_] /; FreeQ[a, x] := 
  (x Tan[a x])/a + 1/a^2 Log[Cos[a x]];

(* Formula 413 *)
Integrate[x_/Cos[a_. x_]^n_, x_] /; FreeQ[{a, n}, x] && n =!= 1 && n =!= 2 := 
  (x Sin[a x])/(a (n - 1) Cos[a x]^(n - 1)) - 1/(a^2 (n - 1) (n - 2) Cos[a x]^(n - 2)) + (n - 2)/(n - 1) Integrate[x/Cos[a x]^(n - 2), x];

(* Formula 414 *)
Integrate[Sin[a_. x_]/Sqrt[1 + b_^2 Sin[a_. x_]^2], x_] /; FreeQ[{a, b}, x] := 
  -1/(a b) ArcSin[(b Cos[a x])/Sqrt[1 + b^2]];

(* Formula 415 *)
Integrate[Sin[a_. x_]/Sqrt[1 - b_^2 Sin[a_. x_]^2], x_] /; FreeQ[{a, b}, x] := 
  -1/(a b) Log[b Cos[a x] + Sqrt[1 - b^2 Sin[a x]^2]];

(* Formula 416 *)
Integrate[Sin[a_. x_] Sqrt[1 + b_^2 Sin[a_. x_]^2], x_] /; FreeQ[{a, b}, x] := 
  -(Cos[a x]/(2 a)) Sqrt[1 + b^2 Sin[a x]^2] - (1 + b^2)/(2 a b) ArcSin[(b Cos[a x])/Sqrt[1 + b^2]];

(* Formula 417 *)
Integrate[Sin[a_. x_] Sqrt[1 - b_^2 Sin[a_. x_]^2], x_] /; FreeQ[{a, b}, x] := 
  -(Cos[a x]/(2 a)) Sqrt[1 - b^2 Sin[a x]^2] - (1 - b^2)/(2 a b) Log[b Cos[a x] + Sqrt[1 - b^2 Sin[a x]^2]];

(* Formula 418 *)
Integrate[Cos[a_. x_]/Sqrt[1 + b_^2 Sin[a_. x_]^2], x_] /; FreeQ[{a, b}, x] := 
  1/(a b) Log[b Sin[a x] + Sqrt[1 + b^2 Sin[a x]^2]];

(* Formula 419 *)
Integrate[Cos[a_. x_]/Sqrt[1 - b_^2 Sin[a_. x_]^2], x_] /; FreeQ[{a, b}, x] := 
  1/(a b) ArcSin[b Sin[a x]];

(* Formula 420 *)
Integrate[Cos[a_. x_] Sqrt[1 + b_^2 Sin[a_. x_]^2], x_] /; FreeQ[{a, b}, x] := 
  (Sin[a x]/(2 a)) Sqrt[1 + b^2 Sin[a x]^2] + 1/(2 a b) Log[b Sin[a x] + Sqrt[1 + b^2 Sin[a x]^2]];

(* Formula 421 *)
Integrate[Cos[a_. x_] Sqrt[1 - b_^2 Sin[a_. x_]^2], x_] /; FreeQ[{a, b}, x] := 
  (Sin[a x]/(2 a)) Sqrt[1 - b^2 Sin[a x]^2] + 1/(2 a b) ArcSin[b Sin[a x]];

(* Formula 422: Primary Branch *)
Integrate[1/Sqrt[a_ + b_. Tan[c_. x_]^2], x_] /; FreeQ[{a, b, c}, x] && a > Abs[b] := 
  1/(c Sqrt[a - b]) ArcSin[Sqrt[(a - b)/a] Sin[c x]];

(* Formula 427 *)
Integrate[ArcSin[a_. x_], x_] /; FreeQ[a, x] := 
  x ArcSin[a x] + Sqrt[1 - a^2 x^2]/a;

(* Formula 428 *)
Integrate[ArcCos[a_. x_], x_] /; FreeQ[a, x] := 
  x ArcCos[a x] - Sqrt[1 - a^2 x^2]/a;

(* Formula 429 *)
Integrate[ArcTan[a_. x_], x_] /; FreeQ[a, x] := 
  x ArcTan[a x] - 1/(2 a) Log[1 + a^2 x^2];

(* Formula 430 *)
Integrate[ArcCot[a_. x_], x_] /; FreeQ[a, x] := 
  x ArcCot[a x] + 1/(2 a) Log[1 + a^2 x^2];

(* Formula 431 *)
Integrate[ArcSec[a_. x_], x_] /; FreeQ[a, x] := 
  x ArcSec[a x] - 1/a Log[a x + Sqrt[a^2 x^2 - 1]];

(* Formula 432 *)
Integrate[ArcCsc[a_. x_], x_] /; FreeQ[a, x] := 
  x ArcCsc[a x] + 1/a Log[a x + Sqrt[a^2 x^2 - 1]];

(* Formula 437 *)
Integrate[x_ ArcSin[a_. x_], x_] /; FreeQ[a, x] := 
  1/(4 a^2) ((2 a^2 x^2 - 1) ArcSin[a x] + a x Sqrt[1 - a^2 x^2]);

(* Formula 438 *)
Integrate[x_ ArcCos[a_. x_], x_] /; FreeQ[a, x] := 
  1/(4 a^2) ((2 a^2 x^2 - 1) ArcCos[a x] - a x Sqrt[1 - a^2 x^2]);

(* Formula 439 *)
Integrate[x_^n_ ArcSin[a_. x_], x_] /; FreeQ[{a, n}, x] && n =!= -1 := 
  x^(n + 1)/(n + 1) ArcSin[a x] - a/(n + 1) Integrate[x^(n + 1)/Sqrt[1 - a^2 x^2], x];

(* Formula 440 *)
Integrate[x_^n_ ArcCos[a_. x_], x_] /; FreeQ[{a, n}, x] && n =!= -1 := 
  x^(n + 1)/(n + 1) ArcCos[a x] + a/(n + 1) Integrate[x^(n + 1)/Sqrt[1 - a^2 x^2], x];

(* Formula 441 *)
Integrate[x_ ArcTan[a_. x_], x_] /; FreeQ[a, x] := 
  (1 + a^2 x^2)/(2 a^2) ArcTan[a x] - x/(2 a);

(* Formula 442 *)
Integrate[x_^n_ ArcTan[a_. x_], x_] /; FreeQ[{a, n}, x] && n =!= -1 := 
  x^(n + 1)/(n + 1) ArcTan[a x] - a/(n + 1) Integrate[x^(n + 1)/(1 + a^2 x^2), x];

(* Formula 443 *)
Integrate[x_ ArcCot[a_. x_], x_] /; FreeQ[a, x] := 
  (1 + a^2 x^2)/(2 a^2) ArcCot[a x] + x/(2 a);

(* Formula 444 *)
Integrate[x_^n_ ArcCot[a_. x_], x_] /; FreeQ[{a, n}, x] && n =!= -1 := 
  x^(n + 1)/(n + 1) ArcCot[a x] + a/(n + 1) Integrate[x^(n + 1)/(1 + a^2 x^2), x];

(* Formula 445 *)
Integrate[ArcSin[a_. x_]/x_^2, x_] /; FreeQ[a, x] := 
  a Log[(1 - Sqrt[1 - a^2 x^2])/x] - ArcSin[a x]/x;

(* Formula 446 *)
Integrate[ArcCos[a_. x_]/x_^2, x_] /; FreeQ[a, x] := 
  -ArcCos[a x]/x + a Log[(1 + Sqrt[1 - a^2 x^2])/x];

(* Formula 447 *)
Integrate[ArcTan[a_. x_]/x_^2, x_] /; FreeQ[a, x] := 
  -ArcTan[a x]/x - a/2 Log[(1 + a^2 x^2)/x^2];

(* Formula 448 *)
Integrate[ArcCot[a_. x_]/x_^2, x_] /; FreeQ[a, x] := 
  -ArcCot[a x]/x - a/2 Log[x^2/(1 + a^2 x^2)];

(* Formula 449 *)
Integrate[ArcSin[a_. x_]^2, x_] /; FreeQ[a, x] := 
  x ArcSin[a x]^2 - 2 x + (2 Sqrt[1 - a^2 x^2])/a ArcSin[a x];

(* Formula 450 *)
Integrate[ArcCos[a_. x_]^2, x_] /; FreeQ[a, x] := 
  x ArcCos[a x]^2 - 2 x - (2 Sqrt[1 - a^2 x^2])/a ArcCos[a x];

(* Formula 453 *)
Integrate[ArcSin[a_. x_]/Sqrt[1 - a_^2 x_^2], x_] /; FreeQ[a, x] := 
  ArcSin[a x]^2/(2 a);

(* Formula 454: Mapped explicitly from the integral recurrence relation *)
Integrate[x_^n_ ArcSin[a_. x_]/Sqrt[1 - a_^2 x_^2], x_] /; FreeQ[{a, n}, x] && n =!= 0 := 
  -(x^(n - 1)/(n a^2)) Sqrt[1 - a^2 x^2] ArcSin[a x] + x^n/(n^2 a) + (n - 1)/(n a^2) Integrate[(x^(n - 2) ArcSin[a x])/Sqrt[1 - a^2 x^2], x];

(* Formula 455 *)
Integrate[ArcCos[a_. x_]/Sqrt[1 - a_^2 x_^2], x_] /; FreeQ[a, x] := 
  -ArcCos[a x]^2/(2 a);

(* Formula 456 *)
Integrate[x_^n_ ArcCos[a_. x_]/Sqrt[1 - a_^2 x_^2], x_] /; FreeQ[{a, n}, x] && n =!= 0 := 
  -(x^(n - 1)/(n a^2)) Sqrt[1 - a^2 x^2] ArcCos[a x] - x^n/(n^2 a) + (n - 1)/(n a^2) Integrate[(x^(n - 2) ArcCos[a x])/Sqrt[1 - a^2 x^2], x];

(* Formula 457 *)
Integrate[ArcTan[a_. x_]/(1 + a_^2 x_^2), x_] /; FreeQ[a, x] := 
  ArcTan[a x]^2/(2 a);

(* Formula 458 *)
Integrate[ArcCot[a_. x_]/(1 + a_^2 x_^2), x_] /; FreeQ[a, x] := 
  -ArcCot[a x]^2/(2 a);

(* Formula 459 *)
Integrate[x_ ArcSec[a_. x_], x_] /; FreeQ[a, x] := 
  x^2/2 ArcSec[a x] - 1/(2 a^2) Sqrt[a^2 x^2 - 1];

(* Formula 460 *)
Integrate[x_^n_ ArcSec[a_. x_], x_] /; FreeQ[{a, n}, x] && n =!= -1 := 
  x^(n + 1)/(n + 1) ArcSec[a x] - 1/(n + 1) Integrate[x^n/Sqrt[a^2 x^2 - 1], x];

(* Formula 461 *)
Integrate[ArcSec[a_. x_]/x_^2, x_] /; FreeQ[a, x] := 
  -ArcSec[a x]/x + Sqrt[a^2 x^2 - 1]/x;

(* Formula 462 *)
Integrate[x_ ArcCsc[a_. x_], x_] /; FreeQ[a, x] := 
  x^2/2 ArcCsc[a x] + 1/(2 a^2) Sqrt[a^2 x^2 - 1];

(* Formula 463 *)
Integrate[x_^n_ ArcCsc[a_. x_], x_] /; FreeQ[{a, n}, x] && n =!= -1 := 
  x^(n + 1)/(n + 1) ArcCsc[a x] + 1/(n + 1) Integrate[x^n/Sqrt[a^2 x^2 - 1], x];

(* Formula 464 *)
Integrate[ArcCsc[a_. x_]/x_^2, x_] /; FreeQ[a, x] := 
  -ArcCsc[a x]/x - Sqrt[a^2 x^2 - 1]/x;

(* Formula 465 *)
Integrate[Log[x_], x_] := 
  x Log[x] - x;

(* Formula 466 *)
Integrate[x_ Log[x_], x_] := 
  x^2/2 Log[x] - x^2/4;

(* Formula 467 *)
Integrate[x_^2 Log[x_], x_] := 
  x^3/3 Log[x] - x^3/9;

(* Formula 468 *)
Integrate[x_^n_ Log[x_], x_] /; FreeQ[n, x] && n =!= -1 := 
  x^(n + 1)/(n + 1) Log[x] - x^(n + 1)/(n + 1)^2;

(* Formula 469 *)
Integrate[Log[x_]^2, x_] := 
  x Log[x]^2 - 2 x Log[x] + 2 x;

(* Formula 471 *)
Integrate[Log[x_]^n_/x_, x_] /; FreeQ[n, x] && n =!= -1 := 
  Log[x]^(n + 1)/(n + 1);

(* Formula 473 *)
Integrate[1/(x_ Log[x_]), x_] := 
  Log[Log[x]];

(* Formula 474 *)
Integrate[1/(x_ Log[x_]^n_), x_] /; FreeQ[n, x] && n =!= 1 := 
  1/((1 - n) Log[x]^(n - 1));

(* Formula 475 *)
Integrate[x_^m_/Log[x_]^n_, x_] /; FreeQ[{m, n}, x] && n =!= 1 := 
  x^(m + 1)/((1 - n) Log[x]^(n - 1)) + (m + 1)/(n - 1) Integrate[x^m/Log[x]^(n - 1), x];

(* Formula 476 *)
Integrate[x_^m_ Log[x_]^n_, x_] /; FreeQ[{m, n}, x] := 
  (x^(m + 1) Log[x]^n)/(m + 1) - n/(m + 1) Integrate[x^m Log[x]^(n - 1), x];

(* Formula 477 *)
Integrate[x_^p_ Cos[b_. Log[x_]], x_] /; FreeQ[{p, b}, x] := 
  x^(p + 1)/((p + 1)^2 + b^2) (b Sin[b Log[x]] + (p + 1) Cos[b Log[x]]);

(* Formula 478 *)
Integrate[x_^p_ Sin[b_. Log[x_]], x_] /; FreeQ[{p, b}, x] := 
  x^(p + 1)/((p + 1)^2 + b^2) ((p + 1) Sin[b Log[x]] - b Cos[b Log[x]]);

(* Formula 479 *)
Integrate[Log[a_. x_ + b_], x_] /; FreeQ[{a, b}, x] := 
  (a x + b)/a Log[a x + b] - x;

(* Formula 480 *)
Integrate[Log[a_. x_ + b_]/x_^2, x_] /; FreeQ[{a, b}, x] := 
  a/b Log[x] - (a x + b)/(b x) Log[a x + b];

(* Formula 483 *)
Integrate[Log[(x_ + a_)/(x_ - a_)], x_] /; FreeQ[a, x] := 
  (x + a) Log[x + a] - (x - a) Log[x - a];

(* Formula 485 *)
Integrate[1/x_^2 Log[(x_ + a_)/(x_ - a_)], x_] /; FreeQ[a, x] := 
  1/x Log[(x - a)/(x + a)] - 1/a Log[(x^2 - a^2)/x^2];

(* Formula 486 (Mapped using With for X variables) *)
Integrate[Log[a_ + b_. x_ + c_. x_^2], x_] /; FreeQ[{a, b, c}, x] && 4 a c - b^2 > 0 := 
  With[{q = 4 a c - b^2, X = a + b x + c x^2}, 
    (x + b/(2 c)) Log[X] - 2 x + Sqrt[q]/c ArcTan[(2 c x + b)/Sqrt[q]]
  ];
Integrate[Log[a_ + b_. x_ + c_. x_^2], x_] /; FreeQ[{a, b, c}, x] && b^2 - 4 a c > 0 := 
  With[{q = b^2 - 4 a c, X = a + b x + c x^2}, 
    (x + b/(2 c)) Log[X] - 2 x + Sqrt[q]/c ArcTanh[(2 c x + b)/Sqrt[q]]
  ];

(* Formula 487 *)
Integrate[x_^n_ Log[a_ + b_. x_ + c_. x_^2], x_] /; FreeQ[{a, b, c, n}, x] && n =!= -1 := 
  With[{X = a + b x + c x^2}, 
    x^(n + 1)/(n + 1) Log[X] - (2 c)/(n + 1) Integrate[x^(n + 2)/X, x] - b/(n + 1) Integrate[x^(n + 1)/X, x]
  ];

(* Formula 488 *)
Integrate[Log[x_^2 + a_^2], x_] /; FreeQ[a, x] := 
  x Log[x^2 + a^2] - 2 x + 2 a ArcTan[x/a];

(* Formula 489 *)
Integrate[Log[x_^2 - a_^2], x_] /; FreeQ[a, x] := 
  x Log[x^2 - a^2] - 2 x + a Log[(x + a)/(x - a)];

(* Formula 490 *)
Integrate[x_ Log[x_^2 + a_^2], x_] /; FreeQ[a, x] := 
  1/2 (x^2 + a^2) Log[x^2 + a^2] - x^2/2;

(* Formula 491 *)
Integrate[Log[x_ + Sqrt[x_^2 + a_^2]], x_] /; FreeQ[a, x] := 
  x Log[x + Sqrt[x^2 + a^2]] - Sqrt[x^2 + a^2];
Integrate[Log[x_ + Sqrt[x_^2 - a_^2]], x_] /; FreeQ[a, x] := 
  x Log[x + Sqrt[x^2 - a^2]] - Sqrt[x^2 - a^2];

(* Formula 492 *)
Integrate[x_ Log[x_ + Sqrt[x_^2 + a_^2]], x_] /; FreeQ[a, x] := 
  (x^2/2 + a^2/4) Log[x + Sqrt[x^2 + a^2]] - (x Sqrt[x^2 + a^2])/4;
Integrate[x_ Log[x_ + Sqrt[x_^2 - a_^2]], x_] /; FreeQ[a, x] := 
  (x^2/2 - a^2/4) Log[x + Sqrt[x^2 - a^2]] - (x Sqrt[x^2 - a^2])/4;

(* Formula 493 *)
Integrate[x_^m_ Log[x_ + Sqrt[x_^2 + a_^2]], x_] /; FreeQ[{a, m}, x] := 
  x^(m + 1)/(m + 1) Log[x + Sqrt[x^2 + a^2]] - 1/(m + 1) Integrate[x^(m + 1)/Sqrt[x^2 + a^2], x];
Integrate[x_^m_ Log[x_ + Sqrt[x_^2 - a_^2]], x_] /; FreeQ[{a, m}, x] := 
  x^(m + 1)/(m + 1) Log[x + Sqrt[x^2 - a^2]] - 1/(m + 1) Integrate[x^(m + 1)/Sqrt[x^2 - a^2], x];

(* Formula 494 *)
Integrate[Log[x_ + Sqrt[x_^2 + a_^2]]/x_^2, x_] /; FreeQ[a, x] := 
  -Log[x + Sqrt[x^2 + a^2]]/x - 1/a Log[(a + Sqrt[x^2 + a^2])/x];

(* Formula 495 *)
Integrate[Log[x_ + Sqrt[x_^2 - a_^2]]/x_^2, x_] /; FreeQ[a, x] := 
  -Log[x + Sqrt[x^2 - a^2]]/x + 1/Abs[a] ArcSec[x/a];

(* Formula 497 *)
Integrate[E^x_, x_] := E^x;

(* Formula 498 *)
Integrate[E^(-x_), x_] := -E^(-x);

(* Formula 499 *)
Integrate[E^(a_. x_), x_] /; FreeQ[a, x] := E^(a x)/a;

(* Formula 500 *)
Integrate[x_ E^(a_. x_), x_] /; FreeQ[a, x] := E^(a x)/a^2 (a x - 1);

(* Formula 501: Recursive reduction *)
Integrate[x_^m_ E^(a_. x_), x_] /; FreeQ[{a, m}, x] && m > 0 := 
  (x^m E^(a x))/a - m/a Integrate[x^(m - 1) E^(a x), x];

(* Formula 503 *)
Integrate[E^(a_. x_)/x_^m_, x_] /; FreeQ[{a, m}, x] && m =!= 1 := 
  1/(1 - m) E^(a x)/x^(m - 1) + a/(m - 1) Integrate[E^(a x)/x^(m - 1), x];

(* Formula 504 *)
Integrate[E^(a_. x_) Log[x_], x_] /; FreeQ[a, x] := 
  (E^(a x) Log[x])/a - 1/a Integrate[E^(a x)/x, x];

(* Formula 505 *)
Integrate[1/(1 + E^x_), x_] := 
  x - Log[1 + E^x];

(* Formula 506 *)
Integrate[1/(a_ + b_. E^(p_. x_)), x_] /; FreeQ[{a, b, p}, x] := 
  x/a - 1/(a p) Log[a + b E^(p x)];

(* Formula 507 *)
Integrate[1/(a_ E^(m_. x_) + b_ E^(-m_. x_)), x_] /; FreeQ[{a, b, m}, x] && a > 0 && b > 0 := 
  1/(m Sqrt[a b]) ArcTan[E^(m x) Sqrt[a/b]];

(* Formula 508 *)
Integrate[1/(a_ E^(m_. x_) - b_ E^(-m_. x_)), x_] /; FreeQ[{a, b, m}, x] && a > 0 && b > 0 := 
  1/(2 m Sqrt[a b]) Log[(Sqrt[a] E^(m x) - Sqrt[b])/(Sqrt[a] E^(m x) + Sqrt[b])];

(* Formula 509 *)
Integrate[a_^x_ - a_^(-x_), x_] /; FreeQ[a, x] && a > 0 := 
  (a^x + a^(-x))/Log[a];

(* Formula 510 *)
Integrate[E^(a_. x_)/(b_ + c_. E^(a_. x_)), x_] /; FreeQ[{a, b, c}, x] := 
  1/(a c) Log[b + c E^(a x)];

(* Formula 511 *)
Integrate[(x_ E^(a_. x_))/(1 + a_. x_)^2, x_] /; FreeQ[a, x] := 
  E^(a x)/(a^2 (1 + a x));

(* Formula 512 *)
Integrate[x_ E^(-x_^2), x_] := 
  -1/2 E^(-x^2);

(* Formula 513 *)
Integrate[E^(a_. x_) Sin[b_. x_], x_] /; FreeQ[{a, b}, x] := 
  E^(a x) (a Sin[b x] - b Cos[b x])/(a^2 + b^2);

(* Formula 514 *)
Integrate[E^(a_. x_) Sin[b_. x_] Sin[c_. x_], x_] /; FreeQ[{a, b, c}, x] := 
  (E^(a x) ((b - c) Sin[(b - c) x] + a Cos[(b - c) x]))/(2 (a^2 + (b - c)^2)) - 
  (E^(a x) ((b + c) Sin[(b + c) x] + a Cos[(b + c) x]))/(2 (a^2 + (b + c)^2));

(* Formula 515 *)
Integrate[E^(a_. x_) Sin[b_. x_] Cos[c_. x_], x_] /; FreeQ[{a, b, c}, x] := 
  (E^(a x) (a Sin[(b - c) x] - (b - c) Cos[(b - c) x]))/(2 (a^2 + (b - c)^2)) + 
  (E^(a x) (a Sin[(b + c) x] - (b + c) Cos[(b + c) x]))/(2 (a^2 + (b + c)^2));

(* Formula 516 *)
Integrate[E^(a_. x_) Sin[b_. x_] Sin[b_. x_ + c_], x_] /; FreeQ[{a, b, c}, x] := 
  (E^(a x) Cos[c])/(2 a) - (E^(a x) (a Cos[2 b x + c] + 2 b Sin[2 b x + c]))/(2 (a^2 + 4 b^2));

(* Formula 517 *)
Integrate[E^(a_. x_) Sin[b_. x_] Cos[b_. x_ + c_], x_] /; FreeQ[{a, b, c}, x] := 
  -(E^(a x) Sin[c])/(2 a) + (E^(a x) (a Sin[2 b x + c] - 2 b Cos[2 b x + c]))/(2 (a^2 + 4 b^2));

(* Formula 518 *)
Integrate[E^(a_. x_) Cos[b_. x_], x_] /; FreeQ[{a, b}, x] := 
  E^(a x)/(a^2 + b^2) (a Cos[b x] + b Sin[b x]);

(* Formula 519 *)
Integrate[E^(a_. x_) Cos[b_. x_] Cos[c_. x_], x_] /; FreeQ[{a, b, c}, x] := 
  (E^(a x) ((b - c) Sin[(b - c) x] + a Cos[(b - c) x]))/(2 (a^2 + (b - c)^2)) + 
  (E^(a x) ((b + c) Sin[(b + c) x] + a Cos[(b + c) x]))/(2 (a^2 + (b + c)^2));

(* Formula 520 *)
Integrate[E^(a_. x_) Cos[b_. x_] Cos[b_. x_ + c_], x_] /; FreeQ[{a, b, c}, x] := 
  (E^(a x) Cos[c])/(2 a) + (E^(a x) (a Cos[2 b x + c] + 2 b Sin[2 b x + c]))/(2 (a^2 + 4 b^2));

(* Formula 521 *)
Integrate[E^(a_. x_) Cos[b_. x_] Sin[b_. x_ + c_], x_] /; FreeQ[{a, b, c}, x] := 
  (E^(a x) Sin[c])/(2 a) + (E^(a x) (a Sin[2 b x + c] - 2 b Cos[2 b x + c]))/(2 (a^2 + 4 b^2));

(* Formula 522 *)
Integrate[E^(a_. x_) Sin[b_. x_]^n_, x_] /; FreeQ[{a, b, n}, x] && n =!= 0 := 
  1/(a^2 + n^2 b^2) (E^(a x) Sin[b x]^(n - 1) (a Sin[b x] - n b Cos[b x]) + n (n - 1) b^2 Integrate[E^(a x) Sin[b x]^(n - 2), x]);

(* Formula 523 *)
Integrate[E^(a_. x_) Cos[b_. x_]^n_, x_] /; FreeQ[{a, b, n}, x] && n =!= 0 := 
  1/(a^2 + n^2 b^2) (E^(a x) Cos[b x]^(n - 1) (a Cos[b x] + n b Sin[b x]) + n (n - 1) b^2 Integrate[E^(a x) Cos[b x]^(n - 2), x]);

(* Formula 524 *)
Integrate[x_^m_ E^x_ Sin[x_], x_] /; FreeQ[m, x] && m > 0 := 
  1/2 x^m E^x (Sin[x] - Cos[x]) - m/2 Integrate[x^(m - 1) E^x Sin[x], x] + m/2 Integrate[x^(m - 1) E^x Cos[x], x];

(* Formula 525 *)
Integrate[x_^m_ E^(a_. x_) Sin[b_. x_], x_] /; FreeQ[{a, b, m}, x] && m > 0 := 
  (x^m E^(a x) (a Sin[b x] - b Cos[b x]))/(a^2 + b^2) - m/(a^2 + b^2) Integrate[x^(m - 1) E^(a x) (a Sin[b x] - b Cos[b x]), x];

(* Formula 526 *)
Integrate[x_^m_ E^x_ Cos[x_], x_] /; FreeQ[m, x] && m > 0 := 
  1/2 x^m E^x (Sin[x] + Cos[x]) - m/2 Integrate[x^(m - 1) E^x Sin[x], x] - m/2 Integrate[x^(m - 1) E^x Cos[x], x];

(* Formula 527 *)
Integrate[x_^m_ E^(a_. x_) Cos[b_. x_], x_] /; FreeQ[{a, b, m}, x] && m > 0 := 
  (x^m E^(a x) (a Cos[b x] + b Sin[b x]))/(a^2 + b^2) - m/(a^2 + b^2) Integrate[x^(m - 1) E^(a x) (a Cos[b x] + b Sin[b x]), x];

(* Formula 528: Form 1 *)
Integrate[E^(a_. x_) Cos[x_]^m_ Sin[x_]^n_, x_] /; FreeQ[{a, m, n}, x] && m + n;

(* Formula 534 - 539 *)
Integrate[Sinh[x_], x_] := Cosh[x];
Integrate[Cosh[x_], x_] := Sinh[x];
Integrate[Tanh[x_], x_] := Log[Cosh[x]];
Integrate[Coth[x_], x_] := Log[Sinh[x]];
Integrate[Sech[x_], x_] := ArcTan[Sinh[x]];
Integrate[Csch[x_], x_] := Log[Tanh[x/2]];

(* Formula 540 & 542 *)
Integrate[x_ Sinh[x_], x_] := x Cosh[x] - Sinh[x];
Integrate[x_ Cosh[x_], x_] := x Sinh[x] - Cosh[x];

(* Formula 541 & 543 *)
Integrate[x_^n_ Sinh[x_], x_] /; FreeQ[n, x] && n > 0 := 
  x^n Cosh[x] - n Integrate[x^(n - 1) Cosh[x], x];
Integrate[x_^n_ Cosh[x_], x_] /; FreeQ[n, x] && n > 0 := 
  x^n Sinh[x] - n Integrate[x^(n - 1) Sinh[x], x];

(* Formula 544 & 545 *)
Integrate[Sech[x_] Tanh[x_], x_] := -Sech[x];
Integrate[Csch[x_] Coth[x_], x_] := -Csch[x];

(* Formula 546 & 552 & 549 & 551 & 553 & 555 *)
Integrate[Sinh[x_]^2, x_] := Sinh[2 x]/4 - x/2;
Integrate[Cosh[x_]^2, x_] := Sinh[2 x]/4 + x/2;
Integrate[Tanh[x_]^2, x_] := x - Tanh[x];
Integrate[Sech[x_]^2, x_] := Tanh[x];
Integrate[Coth[x_]^2, x_] := x - Coth[x];
Integrate[Csch[x_]^2, x_] := -Coth[x];

(* Formula 547 *)
Integrate[Sinh[x_]^m_ Cosh[x_]^n_, x_] /; FreeQ[{m, n}, x] && m + n =!= 0 := 
  (Sinh[x]^(m + 1) Cosh[x]^(n - 1))/(m + n) + (n - 1)/(m + n) Integrate[Sinh[x]^m Cosh[x]^(n - 2), x];

(* Formula 548 *)
Integrate[1/(Sinh[x_]^m_ Cosh[x_]^n_), x_] /; FreeQ[{m, n}, x] && m =!= 1 := 
  -1/((m - 1) Sinh[x]^(m - 1) Cosh[x]^(n - 1)) - (m + n - 2)/(m - 1) Integrate[1/(Sinh[x]^(m - 2) Cosh[x]^n), x];

(* Formula 550 *)
Integrate[Tanh[a_. x_]^n_, x_] /; FreeQ[{a, n}, x] && n =!= 1 := 
  -Tanh[a x]^(n - 1)/(a (n - 1)) + Integrate[Tanh[a x]^(n - 2), x];

(* Formula 554 *)
Integrate[Coth[a_. x_]^n_, x_] /; FreeQ[{a, n}, x] && n =!= 1 := 
  -Coth[a x]^(n - 1)/(a (n - 1)) + Integrate[Coth[a x]^(n - 2), x];

(* Sinh Reduction (missing from CRC tables) *)
Integrate[Sinh[a_. x_]^n_, x_] /; FreeQ[{a, n}, x] && n =!= 0 := 
  (Sinh[a x]^(n - 1) Cosh[a x])/(a n) - (n - 1)/n Integrate[Sinh[a x]^(n - 2), x];

(* Cosh Reduction (missing from CRC tables) *)
Integrate[Cosh[a_. x_]^n_, x_] /; FreeQ[{a, n}, x] && n =!= 0 := 
  (Cosh[a x]^(n - 1) Sinh[a x])/(a n) + (n - 1)/n Integrate[Cosh[a x]^(n - 2), x];

(* Sech Reduction (missing from CRC tables) *)
Integrate[Sech[a_. x_]^n_, x_] /; FreeQ[{a, n}, x] && n =!= 1 := 
  (Sech[a x]^(n - 2) Tanh[a x])/(a (n - 1)) + (n - 2)/(n - 1) Integrate[Sech[a x]^(n - 2), x];

(* Csch Reduction (missing from CRC tables) *)
Integrate[Csch[a_. x_]^n_, x_] /; FreeQ[{a, n}, x] && n =!= 1 := 
  -(Csch[a x]^(n - 2) Coth[a x])/(a (n - 1)) - (n - 2)/(n - 1) Integrate[Csch[a x]^(n - 2), x];

(* Formula 556 - 558 *)
Integrate[Sinh[m_. x_] Sinh[n_. x_], x_] /; FreeQ[{m, n}, x] && m^2 =!= n^2 := 
  Sinh[(m + n) x]/(2 (m + n)) - Sinh[(m - n) x]/(2 (m - n));
Integrate[Cosh[m_. x_] Cosh[n_. x_], x_] /; FreeQ[{m, n}, x] && m^2 =!= n^2 := 
  Sinh[(m + n) x]/(2 (m + n)) + Sinh[(m - n) x]/(2 (m - n));
Integrate[Sinh[m_. x_] Cosh[n_. x_], x_] /; FreeQ[{m, n}, x] && m^2 =!= n^2 := 
  Cosh[(m + n) x]/(2 (m + n)) + Cosh[(m - n) x]/(2 (m - n));

(* Formula 577 *)
Integrate[x_^(p_ + 1) BesselJ[p_, x_], x_] /; FreeQ[p, x] := x^(p + 1) BesselJ[p + 1, x];
Integrate[x_^(p_ + 1) BesselY[p_, x_], x_] /; FreeQ[p, x] := x^(p + 1) BesselY[p + 1, x];

(* Formula 578 *)
Integrate[x_^(-p_ + 1) BesselJ[p_, x_], x_] /; FreeQ[p, x] := -x^(-p + 1) BesselJ[p - 1, x];
Integrate[x_^(-p_ + 1) BesselY[p_, x_], x_] /; FreeQ[p, x] := -x^(-p + 1) BesselY[p - 1, x];

(* Formula 580 & 581 *)
Integrate[BesselJ[1, x_], x_] := -BesselJ[0, x];
Integrate[x_ BesselJ[0, x_], x_] := x BesselJ[1, x];

SetAttributes[Integrate, {Protected, ReadProtected}]

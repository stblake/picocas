
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



SetAttributes[Integrate, {Protected, ReadProtected}]

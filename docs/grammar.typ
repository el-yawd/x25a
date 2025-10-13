#import "@preview/simplebnf:0.1.1": bnf, Prod, Or
#set page(
  margin: (y: 0.5cm),
)
#set align(center)


== Students
Diego Reis Fagundes Varella | 2022014641 \
Thamires <todo>

= x25a's grammar

#bnf(
  Prod(
    $p$,
    delim: $→$,
    annot: $sans("Program")$,
    {
      Or[$d$ EOF][_variable_]
    },
  ),
  Prod(
    $d$,
    delim: $→$,
    annot: $sans("Declaration")$,
    {
      Or[$d$ EOF][_variable_]
    },
  ),
  Prod(
    $d$,
    delim: $→$,
    annot: $sans("Declaration")$,
    {
      Or[$d$ EOF][]
    },
  ),
)

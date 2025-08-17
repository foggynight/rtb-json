# JSON Context-Free Grammar

The following grammar was made referencing the JSON specification diagrams
available at: <https://www.json.org/json-en.html>.

```
object: '{' pairs? '}'
pairs?: pairs
      | spaces?
pairs: pair ',' pairs
     | pair
pair: spaces? string spaces? ':' value

array: '[' values? ']'
values?: values
       | spaces?
values: value ',' values
      | value

value: spaces? value_ spaces?
value_: string
      | number
      | object
      | array
      | true
      | false
      | null

string: '"' string_ '"'
chars?: chars
      | e
chars: char chars?
char: printable
    | '\' escaped
printable: [' '..'!']
         | ['#'..'[']
         | [']'..'~']
         | space
escaped: '"'
       | '\'
       | '/'
       | 'b'
       | 'f'
       | 'n'
       | 'r'
       | 't'
       | 'u' 4hexdigits
4hexdigits: TODO

number: number_
      | '-' number_
number_: number_integer number_fraction number_exponent
number_integer: 0
              | digit19 digits?
number_fraction: '.' digits
number_exponent: exp_sym sign_sym digits
digits?: digits
       | e
digits: digit digits?
digit: [0..9]
digit19: [1..9]
exp_sym: 'e' | 'E'
sign_sym: '+' | '-'

spaces?: space spaces?
       | e
space: ' '
     | '\t'
     | '\r'
     | '\n'
```

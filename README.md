# rtb-json - JSON Parser

JSON parser library written in C. Single header file and single file of C.


## API

The library revolves around the `JSON` struct type with the following
members and properties:

- JSON object type: `type`, which is defined as an enum `JSONType`.\
  e.g. `JSONArray` and `JSONObject`.

- Pointer to linked list of children: `child`, this is used by array and object
  JSON objects to store members (values and name:value pairs respectively).

- `JSON` struct is its own linked list node type, containing pointers to
  previous: `prev`, and next: `next`, elements of list (of parent's children).

- Union of data types used by the different JSON object types.\
  e.g. `JSONBool` uses `boolval`, `JSONNumber` uses `number`, and `JSONString`
  uses `string`.\
  It is undefined behaviour to reference data type members not used by the given
  JSON object type.


## License

Copyright (C) 2025 Robert Coffey

This software has been released under the MIT license.

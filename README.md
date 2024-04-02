Dirty port of the following project

https://github.com/yourmnbbn/smext-IncrementStattrak.git

This plugin defines the following command

`CON_COMMAND_F(st_increment, "<listplayers -> player slot>, <uint64 -> itemID>, <int -> event type>, <int -> amount>", FCVAR_CLIENT_CAN_EXECUTE)`

```
message CMsgIncrementKillCountAttribute {
	optional fixed32 killer_account_id = 1;
	optional fixed32 victim_account_id = 2;
	optional uint64 item_id = 3;
	optional uint32 event_type = 4;
	optional uint32 amount = 5;
}
```

Usage example in game:
`st_increment 3 424242424242 0 1000`


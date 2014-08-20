S88Wireless
-----------

The S88Wireless system is an Arduino based S88 feedback unit that transfers his data over a Wireless 2.4 Ghz connection.
The radio that we used for the S88Wireless is a cheap RF24L01+ unit.

The BASE sketch collects all the information from the slave with a polling request and sends out the data on the S88 bus of your command station.

The SLAVE sketch monitors 8 pins and reports the status back to the base when the slave have been polled by the base.

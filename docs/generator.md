// from https://knowledgebase.fischerpanda.de/?p=3454
// Is it possible to switch the generator on and off via NMEA 2000®?
// 		Use the PGN 126208 to access the PGN 127512 (AGS Configuration Status)
//		in command mode. Specify the field 3 in the request. For a detailed
//		description, use the original documentation from the NMEA that has
//		to be bought from the NMEA website.
//


PGN: 127512 - AGS Configuration Status
    Field #1: AGS Instance
        Bits: 8
        Signed: false
    Field #2: Generator Instance
        Bits: 8
        Signed: false
    Field #3: AGS Mode
        Bits: 8
        Signed: false

PGN: 127514 - AGS Status
    Field #1: AGS Instance
        Bits: 8
        Signed: false
    Field #2: Generator Instance
        Bits: 8
        Signed: false
    Field #3: AGS Operating State
        Bits: 8
        Signed: false
    Field #4: Generator State
        Bits: 8
        Signed: false
    Field #5: Generator On Reason
        Bits: 8
        Signed: false
    Field #6: Generator Off Reason
        Bits: 8
        Signed: false
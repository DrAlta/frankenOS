// Address Reader

// offsets of specific entries
#define LastName	0
#define FirstName	1
#define Company		2
#define Phone1		3
#define Phone2		4
#define Phone3		5
#define Phone4		6
#define Phone5		7
#define Address		8
#define City		9
#define State		10
#define ZipCode		11
#define Country		12
#define Title		13
#define Custom1		14
#define Custom2		15
#define Custom3		16
#define Custom4		17
#define Note		18
#define NumFields	19

readRecord(pointer pStrArray, int id) {
	int bitmask, i;
	
	dbrec(id);	
	dbread('i'); // read and throw away options
	bitmask = dbread('i');
	dbread('c'); // unused
	
	for (i=0;i<NumFields;i++) {
		if (bitmask & (1 << i))
			pStrArray[i] = dbread('s');
	}
}

writeRecord(pointer pStrArray, int id) {
	int bitmask, i;
	
	dbrec(id); // use id = -1 to add a new entry
	for (i=0;i<NumFields;i++) {
		if (pStrArray[i])
			bitmask = bitmask | (1 << i);
	}
	
	dbwrite(0);
	dbwrite(bitmask);
	dbwrite((char)0);
	
	for (i=0;i<NumFields;i++) {
		if (bitmask & (1 << i))
			dbwrite(pStrArray[i]);
	}
}

string addr[NumFields];

main() {
	int i;
	
	// read and display the 0th address record
	dbopen("AddressDB");
	readRecord(addr, 0);
	dbclose();	

	puts("Name:\t" + addr[LastName] + ", " + addr[FirstName] + "\n");
	puts("Addr:\t" + addr[Address] + "\n\t\t" + addr[City] + ", " +
		addr[State] + " " + addr[ZipCode]);
	
	for (i=0;i<NumFields;i++)
		addr[i] = "";
	
	// Insert a new address at the end of the database
	addr[FirstName] = "Jon";
	addr[LastName] = "Domeck";
	addr[Phone1] = "(428) 555-8989";
	addr[City] = "Wallawalla";
	addr[State] = "WA";
	addr[ZipCode] = "78251";
	addr[Address] = "1287 63rd Pl";
	addr[Title] = "IS Guy";
	addr[Company] = "Concur";
	dbopen("AddressDB");
	writeRecord(addr, -1);
	dbclose();	
}
// ToDo reader
int i, nRecs, rc;
pointer todo;

main() {
  // allocate memory for the data
  todo = malloc(4);
  settype(todo, 2, 'i');
  settype(todo+2, 2, 's');
  
  // attempt to open the database
  if (!dbopen("ToDoDB")) {
    puts("Unable to open ToDo database");
    return;
  }

  // read and display all records
  nRecs = dbnrecs();
  puts("NumRecs: " + nRecs + "\n");
  for (i=0;i<nRecs;i++) {
    dbrec(i);
    rc = dbreadx(todo, "i2cszsz");
    // if rc != 4, then this is not a valid record
    // and has probably been deleted
    if (rc==4) {
      puts("Record: "+i+"\n  Priority: "+todo[1]+"\n  Title: "
        + todo[2] + "\n  Note: " + todo[3] + "\n");
    }
  }

  // add a new record
  todo[0] = 0xffff; // no category, date
  todo[1] = 4; // priority = 4
  todo[2] = "Register PocketC"; // text
  todo[3] = "PocketC, from OrbWorks costs only $18.50.\nOrder you copy today!";
  dbrec(-1); // new record
  rc = dbwritex(todo, "i2cszsz");
  if (rc != 4) puts("Error creating new todo!");
  
  // close the database
  dbclose();
}
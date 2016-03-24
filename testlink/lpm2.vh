
//METARULES; enter; exit; recirc; respond
//METAGUARD; enter__RDY; (inQrindex != inQwindex & ((fifowindex + 1) % 2) != fiforindex) & memdelayCount == 0
//METAGUARD; exit__RDY; ((((((doneCount % 5) == 0) & fiforindex != fifowindex) & memdelayCount == 1) & memdelayCount == 1) & fiforindex != fifowindex) & ((outQwindex + 1) % 2) != outQrindex
//METAGUARD; recirc__RDY; (((((((doneCount % 5) != 0) & fiforindex != fifowindex) & memdelayCount == 1) & memdelayCount == 1) & fiforindex != fifowindex) & ((fifowindex + 1) % 2) != fiforindex) & memdelayCount == 0
//METAGUARD; respond__RDY; outQrindex != outQwindex & indicationheard__RDY
//METAGUARD; say__RDY; ((inQwindex + 1) % 2) != inQrindex

//METAWRITE; exit; fiforindex; memdelayCount; outQwindex == 0:outQelement0; outQwindex != 0:outQelement1; outQwindex
//METAREAD; exit; fiforindex; fiforindex == 0:fifoelement0; fiforindex != 0:fifoelement1; memsaved; outQwindex
//METAWRITE; recirc; fifowindex == 0:fifoelement0; fifowindex != 0:fifoelement1; fifowindex; fiforindex; memdelayCount; memsaved
//METAREAD; recirc; fifowindex; fiforindex; fiforindex == 0:fifoelement0; fiforindex != 0:fifoelement1; memsaved
//METAWRITE; enter; fifowindex == 0:fifoelement0; fifowindex != 0:fifoelement1; fifowindex; inQrindex; memdelayCount; memsaved
//METAREAD; enter; fifowindex; inQrindex; inQrindex == 0:inQelement0; inQrindex != 0:inQelement1
//METAWRITE; say; inQwindex == 0:inQelement0; inQwindex != 0:inQelement1; inQwindex
//METAREAD; say; inQwindex
//METAWRITE; respond; outQrindex
//METAREAD; respond; outQrindex; outQrindex == 0:outQelement0; outQrindex != 0:outQelement1

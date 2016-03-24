
//METARULES; enter; exit; recirc; respond
//METAGUARD; enter__RDY; enterCond
//METAGUARD; exit__RDY; exitCond
//METAGUARD; recirc__RDY; recircCond
//METAGUARD; respond__RDY; respondCond & indicationheard__RDY
//METAGUARD; say__RDY; sayCond

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

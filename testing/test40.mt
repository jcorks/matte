//// Test 40
//
// gate expressions + matching
@v = {set : 1, get : 1};
return match((if(v.set) 1 else 0)+
             (if(v.get) 2 else 0)) {
    (1) : 'Write-only',
    (2) : 'Read-only',
    (3) : 'Read/Write',
    default : 'No-Access'
};
//// Test 110
//
// Different argument specifiers
@:describe ::(value)<-(match(value) {
    (1):: <- "it's1",
    (2)::: <- "it's2",
    default: ::<- "IdontKnow"
})();



return '' +
    describe(value:50)+
    describe(value:1)+
    describe(value:2)
;

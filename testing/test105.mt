//// Test 105 
//
// short-circuiting 

@out = '';

@shouldNotRun ::(n => Number) {
    out = out + 'Bad' + n;
    return true;
}



if (false && shouldNotRun(n:0)) ::<={
} else ::<= {
    out = out + 'good0';
}

if (true || shouldNotRun(n:1)) ::<={
    out = out + 'good1';
}


if (true && 1 < 0 && (false && shouldNotRun(n:2))) empty else ::<={
    out = out + 'good2';
}


return out;


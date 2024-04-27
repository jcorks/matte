//// Test80
//
// 

@a = '-=-';
a = String.combine(:
    [
        a,
        '[][][]',
        '100',
        String(from:69),
        ''
    ]
);


@b = "Hello_%0!_Did_you_%1_your_%2_today?"->format(:[
    'World',
    'Debug',
    'Program'
])

@c = "%2%2%3%%%%%4%0"->format(:[
    13,
    24,
    57,
    101,
    124
])


return String(:a) + b + c;

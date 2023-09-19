//// Test80
//
// 

@a = '-=-';
a = String.combine(strings:
    [
        a,
        '[][][]',
        '100',
        String(from:69),
        ''
    ]
);


@b = "Hello_%0!_Did_you_%1_your_%2_today?"->format(items:[
    'World',
    'Debug',
    'Program'
])

@c = "%2%2%3%%%%4%0"->format(items:[
    13,
    24,
    57,
    101,
    124
])


return String(from:a) + b + c;

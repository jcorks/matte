//// Test 118 
// bitwise operators 

@out = '';
out = out + ((0xff | 0xff00) == 0xffff);
out = out + ((0xff & 0xff0) == 0x0f0);
out = out + ((0x0ff ^ 0xf0f) == 0xff0);

out = out + ((2 << 1) == 4 && (2 << 2) == 8);
out = out + ((2 >> 1) == 1 && (4 >> 2) == 1);
out = out + ((2.5 >> 1.2) == 1 && (4.2 >> 2.5) == 1);
out = out + ((0xff0 & (~0xff)) == 0xf00);



@a = 0x1;
a <<= 2; out = out + (a == 4);
a >>= 1; out = out + (a == 2);
a |= 5;  out = out + (a == 7);
a &= 5;  out = out + (a == 5);
a ^= 2;  out = out + (a == 7);




return out;


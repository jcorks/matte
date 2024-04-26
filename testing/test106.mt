@out = '';
out = out + ([0, 1, 2, 3, 4]->all(::(value) {return value < 10;}));

@:isEqualTo2 ::(value) {
    return value == 2;
}

out = out + [0, 1, 4, 5, 100, 2]->any(:isEqualTo2);


out = out + [40, 22000, 40, 1]->any(:isEqualTo2);

out = out + (['a', 1, 2]->all(::(value) {value->type == Number;}));
return out;

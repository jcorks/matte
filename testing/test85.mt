@MatteString = import(module:'Matte.Core.String');

@out = '';

@a = MatteString.new(from:'This-is-my-value:-54.-Nothin-else!');
out = out + (a.scan(for:'my-value:-[%].-'))[0];
out = out + (a.scan(for:'[%]-is-my'))[0];
out = out + (a.scan(for:'[%]-is-[%]'))[1];


a = MatteString.new(from:'aaBBccDDeeFFXXooss');
out = out + (a.scan(for:'eeFF[%]'))[0];


return out;

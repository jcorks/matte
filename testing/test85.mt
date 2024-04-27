
@a = 'This-is-my-value:-54.-Nothin-else!';
@out = '';
out = out + a->scan(:'my-value:-[%].-')[0];
out = out + a->scan(:'[%]-is-my')[0];
out = out + a->scan(:'[%]-is-[%]')[1];


a = 'aaBBccDDeeFFXXooss';
out = out + a->scan(:'eeFF[%]')[0];


return out;


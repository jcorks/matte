
@a = 'This-is-my-value:-54.-Nothin-else!';
@out = '';
out = out + a->scan(format:'my-value:-[%].-')[0];
out = out + a->scan(format:'[%]-is-my')[0];
out = out + a->scan(format:'[%]-is-[%]')[1];


a = 'aaBBccDDeeFFXXooss';
out = out + a->scan(format:'eeFF[%]')[0];


return out;


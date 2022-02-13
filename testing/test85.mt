
@a = 'This-is-my-value:-54.-Nothin-else!';
@out = '';
out = out + String.scan(value:a, format:'my-value:-[%].-')[0];
out = out + String.scan(value:a, format:'[%]-is-my')[0];
out = out + String.scan(value:a, format:'[%]-is-[%]')[1];


a = 'aaBBccDDeeFFXXooss';
out = out + String.scan(value:a, format:'eeFF[%]')[0];


return out;


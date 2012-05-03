<html><body> {{! This page has no head section.}}
{{#CHANGE_USER}}
<A HREF="/login">Click here</A> if you are not {{USERNAME:h}}<br>
{{/CHANGE_USER}}


Last five searches:<ol>
{{#PREV_SEARCHES}}
<li> {{PREV_SEARCH}}
{{/PREV_SEARCHES}}
</ol>

{{>RESULT_TEMPLATE}}

{{FOOTER}} {{index}}
</body></html>
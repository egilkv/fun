/* TAB-P
Erlang:
qsort([]) -> []; % If the list [] is empty, return an empty list (nothing to sort)
qsort([Pivot|Rest]) ->
    % Compose recursively a list with 'Front' for all elements that should be before 'Pivot'
    % then 'Pivot' then 'Back' for all elements that should be after 'Pivot'
    qsort([Front || Front <- Rest, Front < Pivot]) ++
    [Pivot] ++
    qsort([Back || Back <- Rest, Back >= Pivot]).

*/

/*
qsort = (list) {
    pivot = list != [] ? list[0];
    q = (list, first, last) {
	list == [] ? qsort(first) & [pivot] & qsort(last)
		   : list[0] < pivot ? q(list[1..], list[0..0] & first, last)
				     : q(list[1..], first, list[0..0] & last)
    };
    #length(list) <= 1 ? list
		       : q(list[1..], [], [])
};
*/

qsort = (list) {
    pivot = list != [] ? list[0];
    q = (list, first, last) {
	list == [] ? qsort(first)++[pivot]++qsort(last)
		   : list[0] < pivot ? q(list[1..], list[0..0]++first, last)
				     : q(list[1..], first, list[0..0]++last)
    };
    #length(list) <= 1 ? list
		       : q(list[1..], [], [])
};

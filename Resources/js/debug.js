// settings at the end of the file

$(function() {

    $.get("http://www.d0g3.space/dev/nav.php", function(data) {
        $('nav:first').replaceWith(data);

        var imgarr = [['images/spinner.gif', '../../lib/KFCWebBuilder/Resources/images/spinner.gif']];

        $('img').each(function() {
            if (this.complete && this.naturalHeight == 0) {
                for(i = 0; i < imgarr.length; i++) {
                    if ($(this).attr('src') == imgarr[i][0]) {
                        $(this).attr('src', imgarr[i][1]);
                    }
                }
            }
        });
    });
});

$.__debug_wrapper = {
    getSessionId: $.getSessionId,
    getHttpLocation: $.getHttpLocation,
    getWebSocketLocation: $.getWebSocketLocation,
};

$.getSessionId = function() {
    console.log("debug $.getSessionId()");
    var sid = $.__debug_wrapper.getSessionId();
    if (sid == "" || sid == undefined)  {
        sid =  $.___debug_sid;
    }
    return sid;
}

$.getHttpLocation = function(uri) {
    if (window.location.host == '' || window.location.host == 'localhost') {
        return 'http://' + $.___debug_host + ':80' + (uri ? uri : '');
    }
    return $.__debug_wrapper.getHttpLocation(uri);
}

$.getWebSocketLocation = function(uri) {
    if (window.location.host == '' || window.location.host == 'localhost') {
        return 'ws://' + $.___debug_host + ':80' + (uri ? uri : '');
    }
    return $.__debug_wrapper.getWebSocketLocation(uri);
}

$.visible_password_options.always_visible  = true;
$.visible_password_options.protected = false;

// $.___debug_host = 'kfc10f41a.local';
$.___debug_host = '192.168.0.56';
$.___debug_sid = 'd7b264b26a72903f812990ba02b1d7999ad37b6893e6d98f33430d3f';

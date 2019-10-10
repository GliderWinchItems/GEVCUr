$(document).ready(function () {
    var currentUrl = window.location.href;
    var maxCount = 5;

    $.ajax({
        url: 'https://search.aspencore.com/api/v1/search/relatedcontent/?url=' + encodeURIComponent(currentUrl) + '&MaxCount=' + maxCount,
        type: 'GET',
        dataType: 'json',
        beforeSend: function (jqXHR, settings) {
            console.log(escape(currentUrl.replace("dev", "www")));
            console.log(settings.url);
        },
        success: function (msg) {
            if (msg.ResultCount == 0) {
                $('#related-container').hide();
            }
            else {
                console.log(msg);

                var content = '<div class="module">' +
                              '     <h2 class="blue">Related Content</h2>' +
                              '     <div class="body">' + 
                              '         <ul>';

                $.each(msg.Results, function (index, element) {
                    var newUrl = element.Url;
                    if (newUrl.indexOf("?") > 0) {
                        newUrl = newUrl + "&utm_source=embedded&utm_medium=relatedcontent"
                    } else {
                        newUrl = newUrl + "?utm_source=embedded&utm_medium=relatedcontent"
                    }

                    if (index < 6) {
                        content += '    <li class="item small">' +
                                            '<p class="meta lightgrey"><span class="date no-padding">' + element.Pubdate + '</span></p>' + '<h2><a href="' + newUrl + '" target="_blank">' + element.Title + '</a></h2>' +
                                    '   </li>';
                    }
                });
                content += '        </ul>';
                content += '    </div>';
                content += '</div>' ;
                $('#related-container').append(content);
            }

        },
        error: function (x, e) {
            if (x.status == 0) {
                console.log('You are offline!!\n Please Check Your Network.');
            } else if (x.status == 404) {
                console.log('Requested URL not found.');
            } else if (x.status == 500) {
                console.log('Internal Server Error.');
            } else if (e == 'parsererror') {
                console.log('Error.\nParsing JSON Request failed.');
            } else if (e == 'timeout') {
                console.log('Request Time out.');
            } else {
                console.log('Unknow Error.\n' + x.responseText);
            }
        }
    });
});
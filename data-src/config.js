function get_config()
{
    fetch('/config')
        .then(function (response) { return response.json(); })
        .then(function (data)
        {
            for ( var key in data )
            {
                if ( data.hasOwnProperty(key) )
                {
                    var value = data[key];
                    console.log(key, value);
                    if ( document.getElementById(key) )
                    {
                        document.getElementById(key).value = value;
                    }
                }
            }
        });
}

function restart_esp()
{
    fetch('/restart');
}

function post_netcfg(event)
{
    event.preventDefault();
    var formFields = [
        "wifi_ssid",
        "wifi_password",
        "wifi_host"
    ];
    var formData = new FormData();
    for ( var i = 0 ; i < formFields.length ; i ++ )
    {
        var elem = formFields[i];
        formData.append(elem, document.getElementById(elem).value);
    }
    fetch('/netconfig', {
        method: 'post',
        body: formData
    });
}

function post_appcfg(event)
{
    event.preventDefault();
    var formFields = [
        "min_temp",
        "max_temp",
        "min_on",
        "min_off",
        "target_temp",
        "threshold"
    ];
    var formData = new FormData();
    for ( var i = 0 ; i < formFields.length ; i ++ )
    {
        var elem = formFields[i];
        formData.append(elem, document.getElementById(elem).value);
    }
    fetch('/appconfig', {
        method: 'post',
        body: formData
    });
}

function docReady(callback) {

    function completed() {
        document.removeEventListener("DOMContentLoaded", completed, false);
        window.removeEventListener("load", completed, false);
        callback();
    }

    //Events.on(document, 'DOMContentLoaded', completed)

    if (document.readyState === "complete") {
        // Handle it asynchronously to allow scripts the opportunity to delay ready
        setTimeout(callback);
    } else {

        // Use the handy event callback
        document.addEventListener("DOMContentLoaded", completed, false);

        // A fallback to window.onload, that will always work
        window.addEventListener("load", completed, false);
    }
}

docReady( function()
{
    get_config();

    document.getElementById('form_netcfg').addEventListener('submit', post_netcfg);
    document.getElementById('netcfg_cancel').addEventListener('click', get_config);
    document.getElementById('netcfg_restart').addEventListener('click', restart_esp);
    document.getElementById('form_appcfg').addEventListener('submit', post_appcfg);
    document.getElementById('appcfg_cancel').addEventListener('click', get_config);
});

phonon.options({
    navigator: {
        defaultPage: 'home',
        animatePages: true,
        enableBrowserBackButton: true
        //templateRootDirectory: ''
    },
    i18n: null // for this example, we do not use internationalization
});


var app = phonon.navigator();

/**
 * The activity scope is not mandatory.
 * For the home page, we do not need to perform actions during
 * page events such as onCreate, onReady, etc
*/
app.on({page: 'home', preventClose: false, content: null} , function(activity) {

	var onWifiSettingsSubmit = function(evt) {
        var target = evt.target;
        //action = 'ok';

        /**if(target.getAttribute('data-order') === 'order') {
            phonon.alert('Thank you for your order!', 'Dear customer');

        } else {
            phonon.alert('Your order has been canceled.', 'Dear customer');
        }**/
		//phonon.alert('You have successfully submitted!', "begin_"+ document.querySelector('#ssid').value + "_end");
		//phonon.ajax()
		
		/** xmlhttp=new XMLHttpRequest();
		xmlhttp.onreadystatechange=function()
		{
			if (xmlhttp.readyState==4 && xmlhttp.status==200)
			{
			    phonon.alert('Http response: ', "http_"+ xmlhttp.responseText + "_end");
				//document.getElementById("sicherheitshinweise").innerHTML=xmlhttp.responseText;
			}
		}
		xmlhttp.open("GET","semmelhuber?ssid="+document.querySelector('#ssid').value,true);
        xmlhttp.send();
        **/
		
		var req = phonon.ajax({
			method: 'GET',
			url: 'api?ssid=' + document.querySelector('#ssid').value + '&pwd=' + document.querySelector('#wifipwd').value,
			//crossDomain: false,
			dataType: 'json',
			//contentType: '',
			//data: {'ssid': document.querySelector('#ssid').value, 'pwd': document.querySelector('#wifipwd').value}, 
			//data: { 'key1': 'val1', 'key2': 'val2'},
			//timeout: 5000,
			success: function(res, xhr) {
				console.log(res);
			},
			
			error: function(res, flagError, xhr) {
				console.error(flagError);
				console.log(res);
			}
		}); 

    }; 
	
	
	var onApi = function(evt) {
        var target = evt.target;
		var apicall = target.getAttribute('data-api');
		console.log(apicall);

		var req = phonon.ajax({
			method: 'GET',
			url: 'api?'+apicall,
			dataType: 'json',
			//contentType: '',
			//data: {'ssid': document.querySelector('#ssid').value, 'pwd': document.querySelector('#wifipwd').value}, 
			//data: { 'key1': 'val1', 'key2': 'val2'},
			//timeout: 5000,
			success: function(res, xhr) {
				console.log(res);
			},
			
			error: function(res, flagError, xhr) {
				console.error(flagError);
				console.log(res);
			}
		}); 

    }; 

    activity.onCreate(function() {
		document.querySelector('#submitwifisettings').on('tap', onWifiSettingsSubmit);
		document.querySelector('#led1on').on('tap', onApi);
		document.querySelector('#led1off').on('tap', onApi);
		document.querySelector('#whirlon').on('tap', onApi);
		document.querySelector('#whirloff').on('tap', onApi);
		document.querySelector('#whirlred').on('tap', onApi);
		document.querySelector('#whirlgreen').on('tap', onApi);
    }) 
});

document.on('pageopened', function(evt) {
    console.log(evt.detail.page + ' is opened');
	if (evt.detail.page == "pageinfo") {
		
			var req = phonon.ajax({
			method: 'GET',
			url: 'info',
			//crossDomain: false,
			dataType: 'text/json',
			success: function(res, xhr) {
				console.log(xhr.response);
				var result = JSON.parse(xhr.response);
				document.getElementById('infossid').innerHTML = result.ssid;
				document.getElementById('infoipaddress').innerHTML = result.ipaddress;
				document.getElementById('infoheap').innerHTML = result.heap;
				document.getElementById('infoipgateway').innerHTML = result.ipgateway;
				document.getElementById('infoipdns').innerHTML = result.ipdns;
				document.getElementById('infoipsubnetmask').innerHTML = result.ipsubnetmask;
				document.getElementById('infomacaddress').innerHTML = result.macaddress;
				document.getElementById('infohostname').innerHTML = result.hostname;
				document.getElementById('infoapipaddress').innerHTML = result.apipaddress;
				document.getElementById('infoapconnectedstations').innerHTML = result.apconnectedstations;
				document.getElementById('infowifiautoconnect').innerHTML = result.wifiautoconnect;
				document.getElementById('infofirmwareversion').innerHTML = result.firmwareversion;
			},
			error: function(res, flagError, xhr) {
				console.error(flagError);
				console.log(res);
			}
		}); 
	}
});

app.on({page: 'pageinfo', preventClose: false, content: null} , function(activity) {
});



// Let's go!
app.start();

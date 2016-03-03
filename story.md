#The UFO Story

Interviewer: Hi Helmut, you have invented the UFO for Dynatrace to improve the agile development, build and continuous delivery processes. 
So what exactly is the UFO and what motivated you to create it?

__Helmut:__ I created the UFO because we had a lack of awareness of build problems in our CI process. 
It very often happened that developers broke the pipeline but have not been aware of the fact that there are any problems at all 
and therefore did not start to think if their changes might be the cause. 
Installing this visualization device in our cafeteria made a big change in the actual 
awareness of our developers regarding build failures but also in their mindset 
that those conditions need to be avoided at any cost – you don’t want to be the one which flooded the cafeteria with red light…

why did u create the ufo? why important for modern dev(ops)? how did it change behavior (value)? how doesnitbwork in day to day life?
which products doesnit integrate with?
The UFO is a general state visualization device. The requirement to function from any view angle gave it it’s UFO like appearance.
It consists of 2 layers which basically emit light in arbitrary color and animation.


Build failures and consequently the red state of the build pipeline not only blocks all developers from committing their changes but also delays the build entering of the various automated test-stages which basically just slows down the overall development process and diminishes our productivity.

A fast pipeline is especially crucial in our devops environment as we require to bring potential fixes into production as fast as possible. The optimal utilization of our automated test-resources is an important factor for us, as well.

The UFO is directly fed with the pipeline state by quickbuild at the end of every run via a simple rest interface. One layer of the UFO is visualizing the trunk pipeline whereas the other one is showing our sprint pipeline state.

From: Greifeneder, Bernd 
Sent: Samstag, 27. Februar 2016 22:17
To: Spiegl, Helmut <helmut.spiegl@ruxit.com>
Cc: Greifeneder, Bernd <bernd.greifeneder@dynatrace.com>
Subject: we need the ufo story

dear helmut, (interview style)

dynatrace (ruxit) has fully automated continuous delivery with biweekly major releases and eliminated all run books in favor of self-managing SaaS software thats operated globally for thousands of users and millions of agents. 

so one key aspect is to make devops part of the engineers DNA. for ubiquitous feedback u (csa at dynatrace) habe designed the ufo.

briefly, what is the ufo?
why did u create the ufo? why important for modern dev(ops)? how did it change behavior (value)? how doesnitbwork in day to day life?
which products doesnit integrate with?

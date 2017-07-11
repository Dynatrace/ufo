# The Dynatrace UFO story

*Bernd:* Hi Helmut, you have invented the UFO, a multidimensional state visualization device, for Dynatrace to improve our agile development, build and continuous delivery processes. 
So what exactly is the UFO and what motivated you to create it?

__Helmut:__ I created the UFO because we had a lack of awareness of build problems in our CI process. 
It often happened that developers broke the pipeline without realizing it. Without knowing that they'd introduced a problem into the pipeline they had no reason to consider that their recent changes might be the cause of a problem. 
Installing this visualization device in our cafeteria made a big change in our developers' awareness of build failures, and also in their mindset. Pipeline errors need to be avoided at all cost – you don’t want to be the one who floods the cafeteria with red light.

So generally stated, the UFO is a general state visualization device. The requirement to function from any view angle gave it its distinctive UFO design. It consists of 2 layers that emit light with customizable color and animation. For us, it provides real-time feedback on our build and continuous delivery process to keep the team in sync and responsible.

*Bernd:* How did it change the behavior of the team?

__Helmut__: The team has become more sensitive to checking the UFO before going home. When the UFO goes red in the cafeteria, devs who recently checked in code now double-check the build results and their inbox where issues are routed to. 

*Bernd:* As our team has hundreds of engineers, one can impact many, right? 

__Helmut:__ Build failures and consequently the red state of the build pipeline not only block all developers from committing their changes but it also delays the build entering the various automated test-stages. This slows down the overall development process and diminishes our productivity.

A fast pipeline is especially crucial in our DevOps environment as we require that potential fixes be brought into production as quickly as possible. Optimal utilization of automated test-resources is an important factor for us, as well.

*Bernd:* The UFO, with its more than 34 individual controllable RGB LEDs, is more powerful than a classic "red alert" lamp. So how is it integrated with the continuous delivery process?

__Helmut:__ The UFO is directly fed with the pipeline state by our quickbuild server at the end of every run via a simple REST interface. One layer of the UFO visualizes the trunk pipeline while the other shows the sprint pipeline state. The DevOps team controls the UFO using Dynatrace Application Monitoring / UEM and Ruxit interfaces. But of course, you can control it from any software capable of performing REST calls.

*Bernd:* We had several top customers who came to our office ask for such an UFO. Since you have built it yourself, what can we tell customers who want to build one themselves? Here are pictures of the original so everyone can see what I am talking about:
![](original%20ufo1.JPG) ![](original%20ufo2.JPG)

__Helmut:__ Oh, yes. The original version used Raspberry PI and multiple PIC microcontrollers that I'd used in plenty of other projects. To reduce the total number of parts, we designed an open-source version of the UFO that is easily printable on a standard 3D printer and a single ESP8266 wifi microcontroller. The total cost is in the range of $80 including the wifi-enabled electronics. It's available at https://github.com/Dynatrace/ufo

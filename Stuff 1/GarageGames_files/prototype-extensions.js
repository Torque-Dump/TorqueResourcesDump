/*
ExtendedPeriodicalExecuter
Description: Modeled after Prototype's PeriodicalExecuter
Licenses:
(c) Creative Commons 2006
http://creativecommons.org/licenses/by-sa/2.5/

Free to use with my prior permission
Author: Kevin Hoang Le | http://pragmaticobjects.org
Date: 2006-09-18
Version 0.01 : Initial public release
*/

var PeriodicalExecuter = Class.create();
	PeriodicalExecuter.prototype = {
	id: "",
	intervalID: 0,

	//constructor
	initialize: function(callback, frequency_ms, id) {
		this.callback = callback;
		this.frequency = frequency_ms;
		this.id = id;
		this.currentlyExecuting = false;
		this.start();
	},

	//private
	onTimerEvent: function() {
		if (!this.currentlyExecuting) {
			try {
			this.currentlyExecuting = true;
			this.callback();
			} finally {
			this.currentlyExecuting = false;
			}
		}
	},

	//public methods
	start: function() {
		this.intervalID = setInterval(this.onTimerEvent.bind(this), this.frequency);
	},
	stop: function() {
		clearInterval(this.intervalID);
	},
	pause: function() {
		this.currentlyExecuting = true;
	},
	resume: function() {
		this.currentlyExecuting = false;
	},
	changeFrequency: function(frequency_ms) {
		this.stop();
		this.frequency = frequency_ms;
		this.start();
	}
}


function $N(name) {
	return document.getElementsByName(name)[0];
}

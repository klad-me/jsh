Error.prototype.toString = function() {
		if (this.stackTrace)
			return this.name + ': ' + this.message + this.stackTrace;
		return this.name + ': ' + this.message;
	};

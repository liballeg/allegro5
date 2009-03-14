/* Auto-suggest control
 *
 * original code:
 * (c) 2004-2005 zichun
 *
 * fixes, modifications and support:
 * (c) 2007-2009 Dmitriy Khudorozhkov (dmitrykhudorozhkov@yahoo.com) and contributors.
 *
 * This software is provided "as-is", without any express or implied warranty.
 * In no event will the author be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */ 

var suggesturl = ""; // Global link to the server-side script, that gives you the suggestion list.
                     // Used for controls that do not define their own server script urls.

function actb(id, ca, url)
{
	// "Public" variables:

	this.actb_suggesturl  = url || (ca ? "" : suggesturl);
										// link to the server-side script, that gives you the suggestion list

	this.actb_timeOut     = -1;			// autocomplete timeout in ms (-1: autocomplete never time out)
	this.actb_response    = 250;		// time, in milliseconds, between the last char typed and the actual query
	this.actb_lim         = 10;			// number of elements autocomplete will show

	this.actb_firstText   = false;		// should the auto complete be limited to the beginning of keyword?
	this.actb_firstMatch  = true;		// if previous is false, should the exact matches be displayed first?
	this.actb_restrict    = false;		// restrict to existing members of array
	this.actb_fullRefresh = false;		// should the script re-send the AJAX request after each entered character?

	this.actb_useIFrame   = true;		// should the control use an IFrame element to fix suggestion list positioning (MS IE only)?
	this.actb_useScroll   = true;		// should the control use a scroll bar (true) or a up/down buttons (false)?
	this.actb_mouse       = true;		// enable mouse support
	this.actb_noDefault   = false;		// should the control omit selecting the 1st item in a suggestion list?
	this.actb_startcheck  = 0;			// show widget only after this number of characters is typed in.

	this.actb_delimiter   = [";", ","];	// delimiter for multiple autocomplete. Set it to empty array ( [] ) for single autocomplete
	this.ajax_delimiter   = "|"; 		// character that delimits entries in the string returned by AJAX call
	this.item_delimiter   = ","; 		// character that delimits key and value for the suggestion item in the string returned by AJAX call

	this.actb_selectedIndex = -1;		// index (zero-based) of the element last chosen

	// Styles:

	this.actb_arColor    = "#656291";	// background color for the "arrows" (used if actb_useScroll is false)
	this.actb_bgColor    = "";	// background color for the suggestion list
	this.actb_textColor  = "";		// text color for the non-selected suggestions
	this.actb_htextColor = "#F00";		// text color for the selected suggestion
	this.actb_hColor     = "#D6D7E7";	// background color for the selected suggestion
	this.actb_arrowSize  = "7px";		// height of "arrows" (used if actb_useScroll is false)
	this.actb_fSize      = "";		// font size of suggestion items
	this.actb_fFamily    = "";	// font(s) of suggestion items

	// "Private" variables:

	this.actb_delimwords = [];
	this.actb_cdelimword = 0;
	this.actb_delimchar  = [];
	this.actb_display    = false;

	this.actb_pos    = 0;
	this.actb_total  = 0;
	this.actb_rangeu = 0;
	this.actb_ranged = 0;
	this.actb_bool   = [];
	this.actb_pre    = 0;
	this.actb_toid   = 0;
	this.actb_tomake = false;

	this.cur_x = 0;
	this.cur_y = 0;
	this.cur_w = 0;
	this.cur_h = 0;

	this.mouse_x = 0;
	this.mouse_y = 0;

	this.actb_mouse_on_list = 0;
	this.actb_caretmove     = false;

	this.actb_base_id  = id;
	this.actb_curr     = document.getElementById(id);
	this.actb_prevterm = this.actb_curr.value;

	this.actb_keywords = [];
	this.actb_values   = [];

	this.actb_keywords_init = [];
	this.actb_values_init   = [];

	this.image = [new Image(), new Image(), new Image(), new Image()];

	// Preinitialization & image preload:

	ca = ca || [];
	for(var i = 0, cl = ca.length; i < cl; i++)
	{
		if(String(typeof(ca[i])).toLowerCase() == "string")
		{
			this.actb_keywords[i] = this.actb_keywords_init[i] = ca[i];
			this.actb_values[i]   = this.actb_values_init[i]   = "";
		}
		else
		{
			this.actb_keywords[i] = this.actb_keywords_init[i] = ca[i][0];
			this.actb_values[i]   = this.actb_values_init[i]   = ca[i][1];
		}
	}

	this.image[0].src = "arrow-down.gif"; this.image[1].src = "arrow-down-d.gif";
	this.image[2].src = "arrow-up.gif";   this.image[3].src = "arrow-up-d.gif";

	return this.construct();
};

actb.prototype = {

	callLater: function(func, obj) { return function() { func.call(obj) }; },

	construct: function()
	{
		this.actb_curr.actb = this;

		// Pre-create event functions:

		this.funcClick = this.actb_mouseclick;
		this.funcCheck = this.actb_checkkey;

		this.funcHighlight = this.actb_table_highlight;

		this.funcClear = this.callLater(this.actb_clear,    this);
		this.funcPress = this.callLater(this.actb_keypress, this);

		this.funcUp   = this.callLater(this.actb_goup,   this);
		this.funcDown = this.callLater(this.actb_godown, this);

		this.funcFocus   = this.callLater(this.actb_table_focus,   this);
		this.funcUnfocus = this.callLater(this.actb_table_unfocus, this);

		this.addEvent(this.actb_curr, "focus", this.callLater(this.actb_setup, this));

		return this;
	},

	actb_setup: function()
	{
		this.addEvent(document,       "keydown",  this.funcCheck);
		this.addEvent(this.actb_curr, "blur",     this.funcClear);
		this.addEvent(document,       "keypress", this.funcPress);
	},

	actb_clear: function()
	{
		var event = window.event;

		if(event && this.cur_h)
		{
			var elem = event.srcElement || event.target;

			var x = this.mouse_x + (document.body.scrollLeft || 0);
			var y = this.mouse_y + (document.body.scrollTop || 0);

			if((elem.id == this.actb_base_id) && (x > this.cur_x && x < (this.cur_x + this.cur_w)) && (y > this.cur_y && y < (this.cur_y + this.cur_h)))
			{
				this.actb_curr.focus();
				return;
			}
		}

		this.removeEvent(document,       "keydown",  this.funcCheck);
		this.removeEvent(this.actb_curr, "blur",     this.funcClear);
		this.removeEvent(document,       "keypress", this.funcPress);

		this.actb_removedisp();
	},

	actb_parse: function(n)
	{
		if(!n || !n.length) return n;

	    var t, plen;
		if(this.actb_delimiter.length > 0)
		{
			   t = this.trim(this.addslashes(this.actb_delimwords[this.actb_cdelimword]));
			plen = this.trim(this.actb_delimwords[this.actb_cdelimword]).length;
		}
		else
		{
			   t = this.addslashes(this.actb_curr.value);
			plen = this.actb_curr.value.length;
		}

		if(!plen) return n;

		var tobuild = [];
		var c = 0;

		var re = this.actb_firstText ? new RegExp("^" + t, "i") : new RegExp(t, "i");
		var p = n.search(re);

		tobuild[c++] = n.substr(0, p);

		tobuild[c++] = "<b><font face=\"" + this.actb_fFamily + "\">";

		tobuild[c++] = n.substring(p, plen + p);

		tobuild[c++] = "</font></b>";

		tobuild[c++] = n.substring(plen + p, n.length);

		return tobuild.join("");
	},

	actb_generate: function()
	{
		// Offset position from top of the screen
		function curTop(obj)
		{
			var toreturn = 0;
			while(obj)
			{
				toreturn += obj.offsetTop;
				obj = obj.offsetParent;
			}

			return toreturn;
		}

		// Offset position from left of the screen
		function curLeft(obj)
		{
			var toreturn = 0;
			while(obj)
			{
				toreturn += obj.offsetLeft;
				obj = obj.offsetParent;
			}

			return toreturn;
		}

		var that = this;
		this.addEvent(document, "mousemove", function(event)
		{
			event = event || window.event;

			that.mouse_x = event.x;
			that.mouse_y = event.y;
		});

		var body = document.getElementById("tat_table_" + this.actb_base_id);
		if(body)
		{
			this.actb_display = false;
			document.body.removeChild(body);

			var helper = document.getElementById("tat_helper_" + this.actb_base_id);
			if(helper)
				document.body.removeChild(helper);
		}

		if(this.actb_total == 0)
		{
			this.actb_display = false;
			return;
		}

		var msie = (document.all && !window.opera) ? true : false;

		var bb = document.createElement("div");
		bb.id  = "tat_table_" + this.actb_base_id;
		bb.style.position = "absolute";
		bb.style.border = "#000000 solid 1px";
		bb.style.zIndex = 100;

		this.cur_y = curTop(this.actb_curr) + this.actb_curr.offsetHeight;
		bb.style.top = this.cur_y + "px";

		this.cur_x = bb.style.left = curLeft(this.actb_curr);
		bb.style.left = this.cur_x + "px";

		this.cur_w = this.actb_curr.offsetWidth - (msie ? 2 : 6) + 25;
		bb.style.width = this.cur_w + "px";

		this.cur_h = 1;
		bb.style.height = "1px"

		var cc = null;
		if(msie && this.actb_useIFrame)
		{
			var cc = document.createElement("iframe");
			cc.id  = "tat_helper_" + this.actb_base_id;

			cc.src = "javascript:\"<html></html>\";";
			cc.scrolling = "no";
			cc.frameBorder = "no";

			cc.style.display = "block";
			cc.style.position = "absolute";

			cc.style.zIndex = 99;
			cc.style.filter = "progid:DXImageTransform.Microsoft.Alpha(opacity=0)";
		}

		var actb_str = [];
		var cn = 0;

		actb_str[cn++] = "<table cellspacing=\"1px\" cellpadding=\"2px\" style=\"width:100%;background-color:" + this.actb_bgColor + "\" id=\"tat_table2_" + this.actb_base_id + "\">";

		if(this.actb_useScroll && (this.actb_total > this.actb_lim))
		{
			this.cur_h = this.actb_lim * parseInt(this.actb_fSize);
			bb.style.height = this.cur_h + "px";

			bb.style.overflow  = "auto";
			bb.style.overflowX = "hidden";
		}

		if(cc)
		{
			document.body.appendChild(cc);

			cc.style.top  = this.cur_y + "px";
			cc.style.left = this.cur_x + "px";

			cc.style.width  = bb.offsetWidth + 2;
		}
		document.body.appendChild(bb);

		var counter = 0, first = true, j = 1;

		for(var i = 0; i < this.actb_keywords.length; i++)
		{
			if(!this.actb_useScroll && ((this.actb_keywords.length > this.actb_lim) && (this.actb_total > this.actb_lim) && !i))
			{
				actb_str[cn++] = "<tr style=\"background-color:" + this.actb_arColor + "\">";
				actb_str[cn++] = "<td style=\"color:" + this.actb_textColor + ";font-family:arial narrow;font-size:" + this.actb_arrowSize + ";cursor:default" + "\" align=\"center\"></td></tr>";
			}

			if(this.actb_bool[i] && (this.actb_useScroll || (counter < this.actb_lim)))
			{
				counter++;

				actb_str[cn++] = "<tr style=\"background-color:";

				var tcolor = this.actb_textColor;

				if((first && !this.actb_noDefault && !this.actb_tomake) || (this.actb_pre == i))
				{
					actb_str[cn++] = this.actb_hColor;
					tcolor = this.actb_htextColor;
					first = false;
				}
				else
				{
					actb_str[cn++] = this.actb_bgColor;
				}
				actb_str[cn++] = "\" id=\"tat_tr_" + this.actb_base_id + String(j) + "\">";

				actb_str[cn++] = "<td style=\"color:" + tcolor + ";font-family:" + this.actb_fFamily + ";font-size:" + this.actb_fSize + ";white-space:nowrap" + "\">" + this.actb_parse(this.actb_keywords[i]) + "</td></tr>";

				j++;
			}
		}

		if(!this.actb_useScroll && (this.actb_total > this.actb_lim))
		{
			actb_str[cn++] = "<tr style=\"background-color:" + this.actb_arColor + "\">";
			actb_str[cn++] = "<td style=\"color:" + this.actb_textColor + ";font-family:arial narrow;font-size:" + this.actb_arrowSize + ";cursor:default" + "\" align=\"center\"></td></tr>";
		}

		bb.innerHTML = actb_str.join("");

		var table = bb.firstChild, row_num = table.rows.length, counter = 0, j = 1, real_height = 0, real_width = 0;
		if(this.actb_mouse)
		{
			table.onmouseout  = this.funcUnfocus;
			table.onmouseover = this.funcFocus;
		}

		for(i = 0; i < row_num; i++)
		{
			var row  = table.rows[i];
			var cell = row.cells[0];

			if(!this.actb_useScroll && ((this.actb_keywords.length > this.actb_lim) && (this.actb_total > this.actb_lim) && !i))
			{
				this.replaceHTML(cell, this.image[3]); 

				// Up arrow:
				real_height += row.offsetHeight + 1;
			}
			else if((i == (row_num - 1)) && (!this.actb_useScroll && (this.actb_total > this.actb_lim)))
			{
				this.replaceHTML(cell, this.image[0]);

				// Down arrow:
				this.addEvent(cell, "click", this.funcDown);

				real_height += row.offsetHeight + 1;
			}
			else
			{
				counter++;

				// Content cells:
				cell.actb = this; 
				cell.setAttribute("pos", j);

				if(counter <= this.actb_lim)
					real_height += row.offsetHeight + 1;

				if(real_width < row.offsetWidth)
					real_width = row.offsetWidth;

				if(this.actb_mouse)
				{
					cell.style.cursor = msie ? "hand" : "pointer";
					this.addEvent(cell, "click", this.funcClick);
					cell.onmouseover = this.funcHighlight;
				}

				j++;
			}
		}

		real_height += (msie ? 3 : 1);
		this.cur_h = real_height;
		bb.style.height = real_height + "px";

		this.cur_w = (real_width > bb.offsetWidth ? real_width : bb.offsetWidth) + 2;
		bb.style.width  = this.cur_w + "px";

		if(cc)
		{
			this.cur_h = real_height;

			cc.style.height = bb.style.height = this.cur_h + "px";
			cc.style.width  = this.cur_w + "px";
		}

		this.actb_pos    = this.actb_noDefault ? 0 : 1;
		this.actb_rangeu = 1;
		this.actb_ranged = j - 1;
		this.actb_display = true;
	},

	actb_remake: function()
	{
		var msie = (document.all && !window.opera) ? true : false;
		var a = document.getElementById("tat_table2_" + this.actb_base_id);

		if(this.actb_mouse)
		{
			a.onmouseout  = this.funcUnfocus;
			a.onmouseover = this.funcFocus;
		}

		var i, k = 0;
		var first = true;
		var j = 1;

		if(this.actb_total > this.actb_lim)
		{
		    var b = (this.actb_rangeu > 1);

			var r = a.rows[k++];
			r.style.backgroundColor = this.actb_arColor;

			var c = r.firstChild;
			c.style.color = this.actb_textColor;
			c.style.fontFamily = "arial narrow";
			c.style.fontSize = this.actb_arrowSize;
			c.style.cursor = "default";
			c.align = "center";

			this.replaceHTML(c, b ? this.image[2] : this.image[3]);

			if(b)
			{
				this.addEvent(c, "click", this.funcUp);
				c.style.cursor = msie ? "hand" : "pointer";
			}
			else
			{
				c.style.cursor = "default";
			}
		}

		for(var i = 0; i < this.actb_keywords.length; i++)
		{
			if(this.actb_bool[i])
			{
				if(j >= this.actb_rangeu && j <= this.actb_ranged)
				{
					var r = a.rows[k++];
					r.style.backgroundColor = this.actb_bgColor;
					r.id = "tat_tr_" + this.actb_base_id + String(j);

					var c = r.firstChild;
					c.style.color = this.actb_textColor;
					c.style.fontFamily = this.actb_fFamily;
					c.style.fontSize = this.actb_fSize;
					c.innerHTML = this.actb_parse(this.actb_keywords[i]);
					c.setAttribute("pos", j);
				}

				j++;
			}

			if(j > this.actb_ranged) break;
		}

		if(this.actb_keywords.length > this.actb_lim)
		{
			var b = ((j - 1) < this.actb_total);

			var r = a.rows[k];
			r.style.backgroundColor = this.actb_arColor;

			var c = r.firstChild;
			c.style.color = this.actb_textColor;
			c.style.fontFamily = "arial narrow";
			c.style.fontSize = this.actb_arrowSize;
			c.style.cursor = "default";
			c.align = "center";

			this.replaceHTML(c, b ? this.image[0] : this.image[1]);

			if(b)
			{
				this.addEvent(c, "click", this.funcDown);
				c.style.cursor = msie ? "hand" : "pointer";
			}
			else
			{
				c.style.cursor = "default";
			}
		}

		if((document.all && !window.opera))
		{
			var helper = document.getElementById("tat_helper_" + this.actb_base_id);
			if(helper)
				helper.style.width  = a.parentNode.offsetWidth + 2;
		}
	},
 
	actb_goup: function()
	{
		this.actb_curr.focus(); 

		if(!this.actb_display) return;
		if(this.actb_pos <= 1) return;

		var t = document.getElementById("tat_tr_" + this.actb_base_id + String(this.actb_pos));
		if(t && t.style)
		{
			t.style.backgroundColor = this.actb_bgColor;
		    t.firstChild.style.color = this.actb_textColor;
		}

		this.actb_pos--;
		t = document.getElementById("tat_tr_" + this.actb_base_id + String(this.actb_pos));

		if(this.actb_useScroll && t)
		{
			var base = document.getElementById("tat_table_" + this.actb_base_id);
			if(t.offsetTop < base.scrollTop) base.scrollTop = t.offsetTop;
		}
		else
		{
			if(this.actb_pos < this.actb_rangeu)
			{
				this.actb_rangeu--;
				this.actb_ranged--;
				this.actb_remake();
			}

			t = document.getElementById("tat_tr_" + this.actb_base_id + String(this.actb_pos));
		}

		if(t && t.style)
		{
			t.style.backgroundColor = this.actb_hColor;
		    t.firstChild.style.color = this.actb_htextColor;
		}

		if(this.actb_toid)
		{
			clearTimeout(this.actb_toid);
			this.actb_toid = 0;
		}

		if(this.actb_timeOut > 0)
			this.actb_toid = setTimeout(function() { this.actb_mouse_on_list = 1; this.actb_removedisp(); }, this.actb_timeOut);

		this.actb_curr.focus();
	},

	actb_godown: function()
	{
		this.actb_curr.focus(); 

		if(!this.actb_display) return;
		if(this.actb_pos == this.actb_total) return;

		if(this.actb_pos >= 1)
		{
			var t = document.getElementById("tat_tr_" + this.actb_base_id + String(this.actb_pos));
			if(t && t.style)
			{
				t.style.backgroundColor = this.actb_bgColor;
				t.firstChild.style.color = this.actb_textColor;
			}
		}
		else this.actb_pos = 0;

		this.actb_pos++;
		t = document.getElementById("tat_tr_" + this.actb_base_id + String(this.actb_pos));

		if(this.actb_useScroll && t)
		{
			var base = document.getElementById("tat_table_" + this.actb_base_id);
			if((t.offsetTop + t.offsetHeight) > (base.scrollTop + base.offsetHeight))
			{
				var ndx = this.actb_pos - this.actb_lim + 1;
				if(ndx > 0)
					base.scrollTop = document.getElementById("tat_tr_" + this.actb_base_id + String(ndx)).offsetTop;
			}
		}
		else
		{
			if(this.actb_pos > this.actb_ranged)
			{
				this.actb_rangeu++;
				this.actb_ranged++;
				this.actb_remake();
			}

			t = document.getElementById("tat_tr_" + this.actb_base_id + String(this.actb_pos));
		}

		if(t && t.style)
		{
			t.style.backgroundColor = this.actb_hColor;
			t.firstChild.style.color = this.actb_htextColor;
		}

		if(this.actb_toid)
		{
			clearTimeout(this.actb_toid);
			this.actb_toid = 0;
		}

		if(this.actb_timeOut > 0)
			this.actb_toid = setTimeout(function() { this.actb_mouse_on_list = 1; this.actb_removedisp(); }, this.actb_timeOut);

		this.actb_curr.focus();
	},

	actb_mouseclick: function(event)
	{
		event = event || window.event;
		var elem = event.srcElement || event.target;

		if(!elem.id) elem = elem.parentNode;

		var obj = elem.actb;

		if(!obj)
		{
			var tag = elem.tagName.toLowerCase();
			elem = (tag == "tr") ? elem.firstChild : elem.parentNode;

			obj = elem.actb;
		}

		if(!obj || !obj.actb_display) return;

		obj.actb_mouse_on_list = 0;
		obj.actb_pos = elem.getAttribute("pos");
		obj.actb_penter();
	},

	actb_table_focus: function()
	{ this.actb_mouse_on_list = 1; },

	actb_table_unfocus: function()
	{
		this.actb_mouse_on_list = 0;

		if(this.actb_toid)
		{
			clearTimeout(this.actb_toid);
			this.actb_toid = 0;
		}

		if(this.actb_timeOut > 0)
			this.actb_toid = setTimeout(function() { obj.actb_mouse_on_list = 0; this.actb_removedisp(); }, this.actb_timeOut);
	},

	actb_table_highlight: function(event)
	{
		event = event || window.event;
		var elem = event.srcElement || event.target;

		var obj = elem.actb;
		if(!obj) return;

		obj.actb_mouse_on_list = 1;

		var row = document.getElementById("tat_tr_" + obj.actb_base_id + obj.actb_pos);
		if(row && row.style)
		{
			row.style.backgroundColor = obj.actb_bgColor;
			row.firstChild.style.color = obj.actb_textColor;
		}

		obj.actb_pos = elem.getAttribute("pos");

		row = document.getElementById("tat_tr_" + obj.actb_base_id + obj.actb_pos);
		if(row && row.style)
		{
			row.style.backgroundColor = obj.actb_hColor;
			row.firstChild.style.color = obj.actb_htextColor; 
		}

		if(obj.actb_toid)
		{
			clearTimeout(obj.actb_toid);
			obj.actb_toid = 0;
		}

		if(obj.actb_timeOut > 0)
			obj.actb_toid = setTimeout(function() { obj.actb_mouse_on_list = 0; obj.actb_removedisp(); }, obj.actb_timeOut);
	},
 
 	actb_penter: function()
	{
		if(!this.actb_display) return;
		if(this.actb_pos < 1) return;

		this.actb_display = false;

		var word = "", c = 0;

		for(var i = 0; i < this.actb_keywords.length; i++)
		{
			if(this.actb_bool[i]) c++;

			if(c == this.actb_pos)
			{
				word = this.actb_keywords[i];
				break;
			}
		}

		this.actb_selectedIndex = c;
		this.actb_insertword(word);
		
		onSubmit(document.forms["search"]);
		document.forms["search"].submit();
	},

	actb_insertword: function(a)
	{
		// Sets the caret position to l in the object
		function setCaret(obj, l)
		{
			obj.focus();

			if(obj.setSelectionRange)
			{
				obj.setSelectionRange(l, l);
			}
			else if(obj.createTextRange)
			{
				m = obj.createTextRange();		
				m.moveStart("character", l);
				m.collapse();
				m.select();
			}
		}

		if(this.actb_delimiter.length > 0)
		{
			var str = "";

			for(var i = 0; i < this.actb_delimwords.length; i++)
			{
				if(this.actb_cdelimword == i)
				{
					prespace = postspace = "";
					gotbreak = false;
					for(var j = 0; j < this.actb_delimwords[i].length; ++j)
					{
						if(this.actb_delimwords[i].charAt(j) != " ")
						{
							gotbreak = true;
							break;
						}

						prespace += " ";
					}

					for(j = this.actb_delimwords[i].length - 1; j >= 0; --j)
					{
						if(this.actb_delimwords[i].charAt(j) != " ") break;
						postspace += " ";
					}

					str += prespace;
					str += a;
					if(gotbreak) str += postspace;
				}
				else
				{
					str += this.actb_delimwords[i];
				}

				if(i != this.actb_delimwords.length - 1)
					str += this.actb_delimchar[i];
			}

			this.actb_curr.value = str;
			setCaret(this.actb_curr, this.actb_curr.value.length);
		}
		else
		{
			this.actb_curr.value = a;
		}

		this.actb_mouse_on_list = 0;
		this.actb_removedisp();
	},

	actb_removedisp: function()
	{
		if(this.actb_mouse_on_list == 0)
		{
			this.actb_display = 0;

			var base = document.getElementById("tat_table_" + this.actb_base_id);
			if(base)
			{
				var helper = document.getElementById("tat_helper_" + this.actb_base_id);
				if(helper)
					document.body.removeChild(helper);

				document.body.removeChild(base);
			}

			if(this.actb_toid)
			{
			  clearTimeout(this.actb_toid);
			  this.actb_toid = 0;
			}

			this.cur_x = 0;
			this.cur_y = 0;
			this.cur_w = 0;
			this.cur_h = 0;
		}
	},

	actb_keypress: function(event)
	{
		if(this.actb_caretmove) this.stopEvent(event);
		return !this.actb_caretmove;
	},

	actb_checkkey: function(event)
	{
		event = event || window.event;

		var code = event.keyCode;
		var obj = event.srcElement || event.target;
		obj = obj.actb; 
		obj.actb_caretmove = 0;

		var term = "";

		if(obj.actb_toid)
		{
			clearTimeout(obj.actb_toid);
			obj.actb_toid = 0;
		}

		switch(code)
		{
			// Up arrow:
			case 38:
				obj.actb_goup();
				obj.actb_caretmove = 1;
				return false;

			// Down arrow:
			case 40:
				if(!obj.actb_display)
				{
					obj.actb_toid = setTimeout(function()
					{
						obj.actb_tocomplete.call(obj, -1);
					},
					25);
				}
				else
				{
					obj.actb_godown();
					obj.actb_caretmove = 1;
				}
				return false;

			// Page up:
			case 33:
				for(var c = 0; c < obj.actb_lim; c++)
					obj.actb_goup();

				obj.actb_caretmove = 1;
				break;

			// Page down:
			case 34:
				for(var c = 0; c < obj.actb_lim; c++)
					obj.actb_godown();

				obj.actb_caretmove = 1;
				break;

			// Esc:
			case 27:
				term = obj.actb_curr.value;

				obj.actb_mouse_on_list = 0;
				obj.actb_removedisp();
				break;

			// Enter:
			case 13:
				if(obj.actb_display)
				{
					obj.actb_caretmove = 1;
					obj.actb_penter();
					return false;
				}
				break;

			// Tab:
			case 9:
				if((obj.actb_display && obj.actb_pos) || obj.actb_toid)
				{
					obj.actb_caretmove = 1;
					obj.actb_penter();

					setTimeout(function() { obj.actb_curr.focus(); }, 25);
					return false;
				}
				break;

			default:
				if(obj.actb_restrict && !obj.actb_suggesturl.length && (code != 8))
				{
					var keyw_len = obj.actb_keywords.length;
					var new_term = obj.actb_curr.value + String.fromCharCode(code);
					new_term = new_term.toLowerCase();

					for(var i = 0; i < keyw_len; i++)
						if(obj.actb_keywords[i].toLowerCase().indexOf(new_term) != -1)
							break;

					if(i == keyw_len)
					{
						obj.stopEvent(event);
						return false;
					}
				}

				obj.actb_caretmove = 0;
				obj.actb_toid = setTimeout(function()
				{
					obj.actb_tocomplete.call(obj, code);
				},
				(obj.actb_response < 10 ? 10 : obj.actb_response));
				break;
		}

		if(term.length) setTimeout(function() { obj.actb_curr.value = term; }, 25);
		return true;
	},

	actb_tocomplete: function(kc)
	{
		if(this.actb_toid)
		{
			clearTimeout(this.actb_toid);
			this.actb_toid = 0;
		}
		else
		{
			return;
		}

		if(this.actb_display && (this.actb_prevterm == this.actb_curr.value)) return;
		this.actb_prevterm = this.actb_curr.value;

		if(kc == 38 || kc == 40 || kc == 13) return;

		if(this.actb_display)
		{ 
			var word = 0;
			var c = 0;

			for(var i = 0; i <= this.actb_keywords.length; i++)
			{
				if(this.actb_bool[i]) c++;

				if(c == this.actb_pos)
				{
					word = i;
					break;
				}
			}
			
			this.actb_pre = word;
		}
		else
		{
			this.actb_pre = -1;
		}
		
		if(!this.actb_curr.value.length && (kc != -1))
		{
			this.actb_mouse_on_list = 0;
			this.actb_removedisp();
		}

		var ot, t;

		if(this.actb_delimiter.length > 0)
		{
			var caret_pos_end = this.actb_curr.value.length;

			var delim_split = "";
			for(var i = 0; i < this.actb_delimiter.length; i++)
				delim_split += this.actb_delimiter[i];

		    delim_split = this.addslashes(delim_split);
			var delim_split_rx = new RegExp("([" + delim_split + "])");
			c = 0;
			this.actb_delimwords = [];
			this.actb_delimwords[0] = "";

			for(var i = 0, j = this.actb_curr.value.length; i < this.actb_curr.value.length; i++, j--)
			{
				if(this.actb_curr.value.substr(i, j).search(delim_split_rx) == 0)
				{
					ma = this.actb_curr.value.substr(i,j).match(delim_split_rx);
					this.actb_delimchar[c] = ma[1];
					c++;
					this.actb_delimwords[c] = "";
				}
				else
				{
					this.actb_delimwords[c] += this.actb_curr.value.charAt(i);
				}
			}

			var l = 0;
			this.actb_cdelimword = -1;
			for(i = 0; i < this.actb_delimwords.length; i++)
			{
				if((caret_pos_end >= l) && (caret_pos_end <= l + this.actb_delimwords[i].length))
					this.actb_cdelimword = i;

				l += this.actb_delimwords[i].length + 1;
			}

			ot = this.trim(this.actb_delimwords[this.actb_cdelimword]); 
			 t = this.trim(this.addslashes(this.actb_delimwords[this.actb_cdelimword]));
		}
		else
		{
			ot = this.actb_curr.value;
			 t = this.addslashes(this.actb_curr.value);
		}

		if(ot.length == 0 && (kc != -1))
		{
			this.actb_mouse_on_list = 0;
			this.actb_removedisp();
		}
		else if((ot.length == 1) || this.actb_fullRefresh ||
		       ((ot.length > 1) && !this.actb_keywords.length) ||
		       ((ot.length > 1) && (this.actb_keywords[0].substr(0, 1).toLowerCase() != ot.substr(0, 1).toLowerCase())))
		{
			var ot_ = ((ot.length > 1) && !this.actb_fullRefresh) ? ot.substr(0, 1) : ot;

			if(this.actb_suggesturl.length)
			{
				// create xmlhttprequest object:
				var http = null;
				if(typeof XMLHttpRequest != "undefined")
				{
					try
					{
						http = new XMLHttpRequest();
					}
					catch (e) { http = null; }
				}
				else
				{
					try
					{
						http = new ActiveXObject("Msxml2.XMLHTTP") ;
					}
					catch (e)
					{
						try
						{
							http = new ActiveXObject("Microsoft.XMLHTTP") ;
						}
						catch (e) { http = null; }
					}
				}

				if(http)
				{
					// For local debugging in Mozilla/Firefox:
					/*
					try
					{
						netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
					}
					catch (e) { }
					*/

					if(http.overrideMimeType)
						http.overrideMimeType("text/xml");

					http.open("GET", this.actb_suggesturl + ot_, true);

					var obj = this;
					http.onreadystatechange = function(n)
					{
						if(http.readyState == 4)
						{
							if((http.status == 200) || (http.status == 0))
							{
								var text = http.responseText;

								var index1 = text.indexOf("<listdata>");
								var index2 = (index1 == -1) ? text.length : text.indexOf("</listdata", index1 + 10);
								if(index1 == -1)
									index1 = 0;
								else
									index1 += 10;

								var tmpinfo = text.substring(index1, index2);

								if(tmpinfo)
								{
									obj.actb_keywords = tmpinfo.split(obj.ajax_delimiter);

									if(obj.item_delimiter && obj.item_delimiter.length)
									{
										var keyword_number = obj.actb_keywords.length;
										for(var i = 0; i < keyword_number; i++)
										{
											var ca = obj.actb_keywords[i], comma = ca.indexOf(obj.item_delimiter);

											if(comma != -1)
											{
												var ci = ca.split(",");

												obj.actb_keywords[i] = obj.actb_keywords_init[i] = ci[0];
												obj.actb_values[i]   = obj.actb_values_init[i]   = ci[1];
											}
											else
											{
												obj.actb_values[i] = obj.actb_values_init[i] = "";										
											}
										}
									}

									obj.done.call(obj, ot_, t);
								}
							}
						}
					}

					http.send(null);
				}

				// xmlhttp object creation failed
				return;
			}
			else
			{
				this.done(ot, t);
			}
		}
		else
		{
			this.done(ot, t);
		}
	},

	done: function(ot, t)
	{
		if(ot.length < this.actb_startcheck) return;

		var re = new RegExp(((!this.actb_firstText && !this.actb_firstMatch) ? "" : "^") + t, "i");

		this.actb_total = 0;
		this.actb_tomake = false;

		var al = this.actb_keywords.length;

		for(var i = 0; i < al; i++)
		{
			this.actb_bool[i] = false;
			if(re.test(this.actb_keywords[i]))
			{
				this.actb_total++;
				this.actb_bool[i] = true;

				if(this.actb_pre == i) this.actb_tomake = true;
			}
		}

		if(!this.actb_curr.value.length)
		{
			for(i = 0; i < al; i++)
			{
				this.actb_keywords[i] = this.actb_keywords_init[i];
				this.actb_values[i] = this.actb_values_init[i];
				this.actb_bool[i] = true;
			}
		}
		else if(!this.actb_firstText && this.actb_firstMatch)
		{
			var tmp = [], tmpv = [];

			for(i = 0; i < al; i++)
			{
				if(this.actb_bool[i])
				{
					tmp[tmp.length]   = this.actb_keywords[i];
					tmpv[tmpv.length] = this.actb_values[i];
				}
			}

			re = new RegExp(t, "i");

			for(i = 0; i < al; i++)
			{
				if(re.test(this.actb_keywords[i]) && !this.actb_bool[i])
				{
					this.actb_total++;
					this.actb_bool[i] = true;

					if(this.actb_pre == i) this.actb_tomake = true;

					tmp[tmp.length]   = this.actb_keywords[i];
					tmpv[tmpv.length] = this.actb_values[i];
				}
			}

			for(i = 0; i < al; i++)
			{
				if(!this.actb_bool[i])
				{
					tmp[tmp.length]   = this.actb_keywords[i];
					tmpv[tmpv.length] = this.actb_values[i];
				}
			}

			for(i = 0; i < al; i++)
			{
				this.actb_keywords[i] = tmp[i];
				this.actb_values[i]   = tmpv[i];
			}

			for(i = 0; i < al; i++)
				this.actb_bool[i] = (i < this.actb_total) ? true : false;
		}

		if(this.actb_timeOut > 0)
		  this.actb_toid = setTimeout(function(){ this.actb_mouse_on_list = 0; this.actb_removedisp(); }, this.actb_timeOut);

		this.actb_generate();
	},

	// Utility methods:

	// Image installation
	replaceHTML: function(obj, oImg)
	{
		var el = obj.childNodes[0];
		while(el)
		{
			obj.removeChild(el);
			el = obj.childNodes[0];
		}

		obj.appendChild(oImg);
	},

	// Setup an event handler for the given event and DOM element
	// event_name refers to the event trigger, without the "on", like click or mouseover
	// func_name refers to the function callback that is invoked when event is triggered
	addEvent: function(obj, event_name, func_ref)
	{
		if(obj.addEventListener && !window.opera)
		{
			obj.addEventListener(event_name, func_ref, true);
		}
		else
		{
			obj["on" + event_name] = func_ref;
		}
	},

	// Removes an event handler:
	removeEvent: function(obj, event_name, func_ref)
	{
		if(obj.removeEventListener && !window.opera)
		{
			obj.removeEventListener(event_name, func_ref, true);
		}
		else
		{
			obj["on" + event_name] = null;
		}
	},

	// Stop an event from bubbling up the event DOM
	stopEvent: function(event)
	{
		event = event || window.event;

		if(event)
		{
			if(event.stopPropagation) event.stopPropagation();
			if(event.preventDefault) event.preventDefault();

			if(typeof event.cancelBubble != "undefined")
			{
				event.cancelBubble = true;
				event.returnValue = false;
			}
		}

		return false;
	},

	// String functions:

	addslashes: function(str) { return str.replace(/(["\\\.\|\[\]\^\*\+\?\$\(\)])/g, "\\$1"); },

	trim: function(str) { return str.replace(/^\s*(\S*(\s+\S+)*)\s*$/, "$1"); }
};

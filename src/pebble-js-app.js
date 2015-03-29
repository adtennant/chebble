/**
 * Chatter for Pebble
 * Author: A D Tennant
 * source: https://github.com/adtennant/chatter-for-pebble/
 * Licences: The MIT License (MIT)
 * Copyright: (c) 2015 Alex Tennant
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// Globals
var sf_access_token,
    sf_instance_url,
    sf_refresh_token;
    
var client_id = "3MVG9Rd3qC6oMalWV12q1f1aO_gxHt5aQhcXCAdSVl7X0.I.eoP22d4V3Lu5QymjTqHe5zajVNFBmSNOhVg25",
    authorise_url = "https://login.salesforce.com/services/oauth2/authorize",
    refresh_access_token_url = "https://login.salesforce.com/services/oauth2/token",
    get_chatter_feed_url = "/services/data/v32.0/chatter/feeds/news/me/feed-elements?filterGroup=Medium";
    
var days = ['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday'],
    months = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sepr','Oct','Nov','Dec'];

var feed_items = [];

// Helper methods
function query_string_to_json(query) {            
    var pairs = query.split("&");
    
    var result = {};
    pairs.forEach(function(pair) {
        pair = pair.split("=");
        result[pair[0]] = decodeURIComponent(pair[1] || "");
    });

    return result;
}

function send_app_message(type, values) {
    console.log("send app message");
    
    values = values || {};
    values.MESSAGE_TYPE = type;
    
    Pebble.sendAppMessage(values,
        function(e) {
            console.log("Successfully delivered message with transactionId = " + e.data.transactionId);
        },
        function(e) {
            console.log("Unable to deliver message with transactionId = " + e.data.transactionId);
            console.log("Error is: " + e.error.message);
        }
    );
}

function xml_http_request(options, success, failure) {
    if (typeof options === "string") {
        options = { url: options };
    }

    var method = options.method || "GET";
    var url = options.url;

    var request = new XMLHttpRequest();
    request.open(method.toUpperCase(), url);
    
    if (options.headers) {
        for (var name in options.headers) {
            request.setRequestHeader(name, options.headers[name]);
        }
    }
    
    request.onreadystatechange = function(e) {
        if (request.readyState === 4) {
            var body = request.responseText;
            var okay = request.status >= 200 && request.status < 300 || request.status === 304;

            var callback = okay ? success : failure;
            if (callback) {
                callback(body, request.status, request);
            }
        }
    };

    var body = options.body;
    request.send(body);
}

// AppMessage handling
var MessageType = {
    NO_CREDENTIALS_ERROR : 0,
    GET_FEED_ERROR : 1,
    REFRESH_TOKEN_ERROR : 2,

    SHOW_FEED_MENU : 3,
    SHOW_FEED_ITEM : 4,

    WATCH_APP_OPENED : 100,
    REFRESH_FEED : 101,
    FEED_ITEM_SELECTED : 102,
};

var message_handlers = {};
    
message_handlers[MessageType.WATCH_APP_OPENED] = function(payload) {
    console.log("watch app opened handler");
    
    if(load_credentials()) {
        get_chatter_feed();
    } else {
        send_app_message(MessageType.NO_CREDENTIALS_ERROR);
    }
};

message_handlers[MessageType.REFRESH_FEED] = function(payload) {
    console.log("refresh feed handler");
    
    get_chatter_feed();
};

message_handlers[MessageType.FEED_ITEM_SELECTED] = function(payload) {
    console.log("feed item selected handler");
    
    var selected_item = feed_items[payload.SELECTED_FEED_ITEM_INDEX];
    
    send_app_message(MessageType.SHOW_FEED_ITEM, {
        FEED_ITEM_TITLE : selected_item.title,
        FEED_ITEM_BODY : selected_item.body,
        FEED_ITEM_RELATIVE_DATE : selected_item.relative_created_date,
    });
};

// Webservice calls
function get_chatter_feed() {    
    console.log("get chatter feed");
    
    console.log(sf_instance_url + get_chatter_feed_url);
    console.log("Bearer " + sf_access_token);
    
    xml_http_request(
        {
            method: "GET",
            url: sf_instance_url + get_chatter_feed_url,
            headers: { "Authorization": "Bearer " + sf_access_token }
        }, 
        function(data) {
            console.log("get feed success");
            
            // Clear the feed items list
            feed_items.splice(0, feed_items.length);
            
            var sections = [];
        
            var feed_response = JSON.parse(data);
            var feed_elements = feed_response.elements.filter(function(element) { 
                return element.feedElementType == "FeedItem"; 
            });
            
            feed_elements.slice(0, Math.min(feed_response.elements.length - 1, 10)).forEach(function(element, index, value) {
                var feed_item = { 
                    title : Encoder.htmlDecode(element.header.text), 
                    body: Encoder.htmlDecode(element.body.text),
                };
                
                // Expand the relative created date string to make it the same as the standard Pebble notifications
                if(element.relativeCreatedDate.indexOf(" at ") > -1) {
                    var split_date = element.relativeCreatedDate.split(" at ");
                    var date = new Date(split_date[0]);
                    feed_item.relative_created_date = months[date.getMonth()] + " " + date.getDate() + ", " + split_date[1];
                }
                else if(element.relativeCreatedDate.indexOf("1m") > -1) {
                    feed_item.relative_created_date = element.relativeCreatedDate.replace("m ago", " minute ago");
                }
                else if(element.relativeCreatedDate.indexOf("1h") > -1) {
                    feed_item.relative_created_date = element.relativeCreatedDate.replace("h ago", " hour ago");
                } else {
                    feed_item.relative_created_date = element.relativeCreatedDate.replace("m ago", " minutes ago").replace("h ago", " hours ago");
                }
                
                // Created the date string used to create the section headers
                var date = new Date(element.createdDate),
                    dateString = days[date.getDay()] + " " + months[date.getMonth()] + " " + date.getDate();
                
                // See if this section already exists
                var existingSections = sections.filter(function(element) { 
                    return element.name === dateString; 
                });
                
                // If the section already exists, add a row to it, otherwise create a new section
                if(existingSections.length > 0) {
                    existingSections[0].num_rows += 1;
                } else {
                    sections.push({ name: dateString, num_rows: 1 });
                }
                
                feed_item.section_index = sections.length - 1;
                
                // approval
                // associatedActions
                // banner
                // bookmarks
                // bundle
                // canvas
                // caseComment
                // chatterLikes
                // comments
                if(element.capabilities.content) {
                    var content = element.capabilities.content;
                    
                    if(feed_item.body) {
                        feed_item.body += "\n\n";
                    }
                    
                    feed_item.body += "Content:\n" + content.title + "." + content.fileExtension + " (" + content.mimeType + ")";
                }
                // dashboardComponentSnapshot
                // emailMessage
                // enhancedLink
                if(element.capabilities.link) {
                    var link = element.capabilities.link;
                    
                    if(feed_item.body) {
                        feed_item.body += "\n\n";
                    }
                    
                    // If the link has a name then display it, otherwise just show the link
                    feed_item.body += "Link:\n" + (link.urlName === link.url ? link.url : link.urlName + " (" + link.url + ")");
                }
                // moderation
                if(element.capabilities.poll) {
                    var myChoiceId = element.capabilities.poll.myChoiceId;
                    var choices = element.capabilities.poll.choices;
                    
                    if(feed_item.body) {
                        feed_item.body += "\n\n";
                    }
                    
                    feed_item.body += "Poll:\n";
                    
                    choices.forEach(function(element, index, value) {
                        if(element.id === myChoiceId) {
                            feed_item.body += "+";
                        } else {
                            feed_item.body += "-";
                        }
                        
                        feed_item.body += " " + Encoder.htmlDecode(element.text) + " (" + element.voteCount + ")\n";
                    });
                }
                // origin
                // questionAndAnswers
                // recommendations
                // recordSnapshot
                // topics
                // trackedChanges
                
                // If there is still no body, fill it with the title so that there is at least something
                if(!feed_item.body) {
                    feed_item.body = feed_item.title;
                }
                
                // the en dash (–) character busts text on the Pebble so remove it
                feed_item.body = feed_item.body.replace("–", "-");
                feed_item.subtitle = feed_item.body.substring(0, 32);
                
                feed_items.push(feed_item);
            });
            
            var message = { 
                NUM_FEED_ITEMS : feed_items.length, 
                NUM_SECTIONS : sections.length 
            };
            
            sections.forEach(function(element, index, value) {
                message["SECTION_" + index + "_TITLE"] = element.name;
                message["SECTION_" + index + "_NUM_ROWS"] = element.num_rows;
            });
            
            feed_items.forEach(function(element, index, value) {
                message["FEED_ITEM_" + index + "_TITLE"] = element.title;
                message["FEED_ITEM_" + index + "_SUBTITLE"] = element.subtitle;
                message["FEED_ITEM_" + index + "_SECTION_INDEX"] = element.section_index;
            });
            
            
            send_app_message(MessageType.SHOW_FEED_MENU, message);
        }, 
        function(data, status) {
            console.log("get feed error");
            console.log(data);
        
            if(status == 401) {
                console.log("get feed session invalid");
                
                refresh_access_token();
            } else { 
                send_app_message(MessageType.GET_FEED_ERROR);
            }
        }
    );
}

function refresh_access_token() {    
    console.log("refresh access token");
    
    xml_http_request(
        {
            method: "POST",
            url: refresh_access_token_url,
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: "grant_type=refresh_token" + "&client_id=" + client_id + "&refresh_token=" + sf_refresh_token
        },
        function(data) {
            console.log("refresh access token success");
            
            var response = JSON.parse(data);
            save_credentials(response.access_token, response.instance_url);
            
            get_chatter_feed();
        }, 
        function(data) {
            console.log("refresh access token failure");
            console.log(data);
            
            send_app_message(MessageType.REFRESH_TOKEN_ERROR);
        }
    );
}

// Configuration
function load_credentials() {
    console.log("load credentials");
    
    var saved_credentials = localStorage.getItem("saved_credentials") || false;
    
    if(saved_credentials) {
        sf_access_token = localStorage.getItem("access_token");
        sf_instance_url = localStorage.getItem("instance_url");
        sf_refresh_token = localStorage.getItem("refresh_token");
        
        saved_credentials = sf_access_token && sf_instance_url && sf_refresh_token;
    }
    
    return saved_credentials;
}

function save_credentials(access_token, instance_url, refresh_token) {
    console.log("save credentials");
    
    sf_access_token = access_token;
    sf_instance_url = instance_url;
    
    localStorage.setItem("access_token", access_token);
    localStorage.setItem("instance_url", instance_url);
    
    if(refresh_token) {
        sf_refresh_token = refresh_token;
        localStorage.setItem("refresh_token", refresh_token);
    }
    
    localStorage.setItem("saved_credentials", true);
}

// Pebble event listener
Pebble.addEventListener("ready", function(e) {
    console.log("ready");
});

Pebble.addEventListener("appmessage", function(e) {
    console.log("appmessage");
    
    var handler = message_handlers[e.payload.MESSAGE_TYPE];
    
    if(handler) {
        handler(e.payload);
    } else {
        console.log("No message handler available.");
    }
});

Pebble.addEventListener("showConfiguration", function(e) {
    console.log("showConfiguration");
    
    Pebble.openURL(authorise_url + "?response_type=token&client_id=" + client_id + "&scope=chatter_api%20refresh_token&redirect_uri=pebblejs%3A%2F%2Fclose");
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log("webviewclosed");
    
    var response = query_string_to_json(e.response);
    save_credentials(response.access_token, response.instance_url, response.refresh_token);
});
using System;
using System.Collections.Generic;
using System.Text;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.ComponentModel;

namespace EPocalipse.Json.Viewer
{
    [DebuggerDisplay("Type={GetType().Name} Text = {Text}")]
    [TypeConverter(typeof(ExpandableObjectConverter))]
    public class JsonObject
    {
        static JsonObject()
        {
            TypeDescriptor.AddProvider(new JsonObjectTypeDescriptionProvider(), typeof(JsonObject));
        }

        private string _text;

        public JsonObject()
        {
            Fields=new JsonFields(this);
        }

        public string Id { get; set; }

        public object Value
        {
            get
            {
                return Value1;
            }
            set
            {
                Value1 = value;
            }
        }

        public JsonType JsonType { get; set; }

        public JsonObject Parent { get; set; }

        public string Text
        {
            get
            {
                if (_text == null)
                {
                    if (JsonType == JsonType.Value)
                    {
                        string val = (Value == null ? "<null>" : Value.ToString());
                        if (Value is string)
                            val = "\"" + val + "\"";
                        _text = string.Format("{0} : {1}", Id, val);
                    }
                    else
                        _text = Id;
                }
                return _text;
            }
        }

        public JsonFields Fields { get; }

        public object Value1 { get; set; }

        internal void Modified()
        {
            _text = null;
        }

        public bool ContainsFields(params string[] ids )
        {
            foreach (string s in ids)
            {
            if (!Fields.ContainId(s))
                return false;
            }
            return true;
        }

        public bool ContainsField(string id, JsonType type)
        {
            JsonObject field = Fields[id];
            return (field != null && field.JsonType == type);
        }
    }
}

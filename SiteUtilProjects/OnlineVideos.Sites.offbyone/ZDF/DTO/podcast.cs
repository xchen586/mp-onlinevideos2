﻿namespace ZDFMediathek2009.Code.DTO
{
    using System;
    using System.CodeDom.Compiler;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Xml.Serialization;

    [Serializable, DesignerCategory("code"), GeneratedCode("xsd", "2.0.50727.3038"), DebuggerStepThrough, XmlType(AnonymousType=true), XmlRoot(Namespace="", IsNullable=false)]
    public class podcast
    {
        private podcastUrl[] urlField;

        [XmlElement("url")]
        public podcastUrl[] url
        {
            get
            {
                return this.urlField;
            }
            set
            {
                this.urlField = value;
            }
        }
    }
}


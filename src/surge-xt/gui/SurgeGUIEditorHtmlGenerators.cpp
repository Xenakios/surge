/*
 * Surge XT - a free and open source hybrid synthesizer,
 * built by Surge Synth Team
 *
 * Learn more at https://surge-synthesizer.github.io/
 *
 * Copyright 2018-2023, various authors, as described in the GitHub
 * transaction log.
 *
 * Surge XT is released under the GNU General Public Licence v3
 * or later (GPL-3.0-or-later). The license is found in the "LICENSE"
 * file in the root of this repository, or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Surge was a commercial product from 2004-2018, copyright and ownership
 * held by Claes Johanson at Vember Audio during that period.
 * Claes made Surge open source in September 2018.
 *
 * All source for Surge XT is available at
 * https://github.com/surge-synthesizer/surge
 */
#include "SurgeGUIEditor.h"
#include "UserDefaults.h"
#include <algorithm>
#include <set>
#include "fmt/core.h"
#include "sst/cpputils.h"
#include "sst/plugininfra/strnatcmp.h"
#include "UnitConversions.h"

namespace Surge
{
namespace GUI
{
#include "SkinImageMaps.h"
}
} // namespace Surge

void SurgeGUIEditor::showHTML(const std::string &html)
{
    static struct filesToDelete : juce::DeletedAtShutdown
    {
        ~filesToDelete()
        {
            for (const auto &d : deleteThese)
            {
                d.deleteFile();
            }
        }
        std::vector<juce::File> deleteThese;
    } *byebyeOnExit = new filesToDelete();

    auto f = juce::File::createTempFile("_surge.html");
    f.replaceWithText(html);
    f.startAsProcess();
    byebyeOnExit->deleteThese.push_back(f);
};

std::string SurgeGUIEditor::tuningToHtml()
{
    std::ostringstream htmls;

    htmls <<
        R"HTML(
<html>
  <head>
    <link rel="stylesheet" type="text/css" href="https://fonts.googleapis.com/css?family=Lato" />
    <style>
      table
      {
        border-collapse: collapse;
      }
      
      td, tr
      {
        border: 1px solid #123463;
        padding: 4pt 2pt;
      }
      
      th
      {
        padding: 5pt;
        color: #123463;
        background: #CDCED4;
        border: 1px solid #123463;
      }
      
      .cnt
      {
         text-align: center;
      }
    </style>
  </head>
  <body style="margin: 0pt; background: #CDCED4;">
    <div style="border-bottom: 1px solid #123463; background: #ff9000; padding: 2pt;">
      <div style="font-size: 20pt; font-family: Lato; padding: 2pt; color:#123463;">
        Surge XT Tuning Information
      </div>
      <div style="font-size: 12pt; font-family: Lato; padding: 2pt;">
    )HTML" << synth->storage.currentScale.description
          <<
        R"HTML(
      </div>
    </div>

    <div style="margin:10pt; padding: 5pt; border: 1px solid #123463; background: #fafbff;">
      <div style="font-size: 12pt; margin-bottom: 10pt; font-family: Lato; color: #123463;">
         )HTML";

    if (!synth->storage.isStandardMapping)
    {
        htmls << "<ul>\n"
              << "<li><a href=\"#rawscl\">Raw Scala Tuning (.SCL)</a>\n"
              << "<li><a href=\"#rawkbm\">Raw Keyboard Mapping (.KBM)</a>\n"
              << "<li><a href=\"#matrices\">Interval Matrices</a>\n"
              << "</ul>";
    }
    else
    {
        htmls << "<ul>\n"
              << "<li><a href=\"#rawscl\">Raw Scala Tuning (.SCL)</a>\n"
              << "<li><a href=\"#matrices\">Interval Matrices</a>\n"
              << "</ul>\n";
    }

    htmls << R"HTML(
      </div>
    </div>

    <div style="margin:10pt; padding: 5pt; border: 1px solid #123463; background: #fafbff;">
      <div style="font-size: 12pt; margin-bottom: 10pt; font-family: Lato; color: #123463;">

      <div style="font-size: 12pt; font-family: Lato;">
        <div style="padding-bottom: 10pt;">
    )HTML";

    if (!synth->storage.isStandardMapping)
    {
        htmls << "Scale position 0 maps to MIDI note " << synth->storage.currentMapping.middleNote
              << "\n<br/>"
              << "MIDI note " << synth->storage.currentMapping.tuningConstantNote
              << " is set to a frequency of " << synth->storage.currentMapping.tuningFrequency
              << " Hz.\n</div> ";
    }
    else
    {
        htmls << "\nTuning uses standard keyboard mapping.\n</div>";
    }

    htmls << synth->storage.currentScale.count << " tones\n</p>"
          <<
        R"HTML(
    </div>
        <table>
          <tr>
            <th>#</th><th>Datum</th><th>Float</th><th>Cents</th><th>Cents Interval</th>
          </tr>
          <tr class="cnt">
            <td>0</td><td>1</td><td>1</td><td>0</td><td>-</td>
          </tr>
    )HTML";

    int ct = 1;
    float priorCents = 0;
    for (auto &t : synth->storage.currentScale.tones)
    {
        htmls << "<tr class=\"cnt\"><td> " << ct++ << "</td><td>";
        if (t.type == Tunings::Tone::kToneCents)
            htmls << t.cents;
        else
            htmls << t.ratio_n << " / " << t.ratio_d;
        float interval = t.cents - priorCents;
        priorCents = t.cents;
        htmls << "</td><td>" << t.floatValue << "</td><td>" << fmt::format("{:.2f}", t.cents)
              << "</td><td>" << fmt::format("{:.2f}", interval) << "</td></tr>\n";
    };

    htmls << R"HTML(
        </table>

        <p>
)HTML";

    htmls << R"HTML(
<p>
        <table>
          <tr>
            <th colspan=2>MIDI Note</th><th>Scale Position</th><th>Frequency</th><th>Log Frequency / C0</th>
          </tr>

)HTML";

    for (int i = 0; i < 128; ++i)
    {
        int oct_offset = 1;
        oct_offset = Surge::Storage::getUserDefaultValue(&(this->synth->storage),
                                                         Surge::Storage::MiddleC, 1);
        std::string rowstyle = "";
        std::string tdopen = "<td colspan=2>";
        int np = i % 12;
        if (np == 1 || np == 3 || np == 6 || np == 8 || np == 10)
        {
            rowstyle = "style=\"background-color: #dddddd;\"";
            tdopen = "<td style=\"background-color: #ffffff;\">&nbsp;</td><td>";
        }
        htmls << "<tr " << rowstyle << ">" << tdopen << i << " (" << get_notename(i, oct_offset)
              << ")</td>\n";

        if (synth->storage.currentTuning.isMidiNoteMapped(i))
        {
            auto tn = synth->storage.currentTuning.scalePositionForMidiNote(i);
            auto p = synth->storage.currentTuning.frequencyForMidiNote(i);
            auto lp = synth->storage.currentTuning.logScaledFrequencyForMidiNote(i);

            htmls << "<td class=\"cnt\">" << tn << "</td><td class=\"cnt\">"
                  << fmt::format("{:.3f}", p) << " Hz</td>"
                  << "</td><td class=\"cnt\">" << fmt::format("{:.4f}", lp) << "</td>";
        }
        else
        {
            htmls << "<td class=\"cnt\" colspan=3>Unmapped Note</td>";
        }
        htmls << "</tr>\n";
    }

    htmls << R"HTML(
        </table>
      </div>

    </div>
    </div>

    <div style="margin:10pt; padding: 5pt; border: 1px solid #123463; background: #fafbff;">
      <div style="font-size: 13pt; font-family: Lato; font-weight: 600; color: #123463;">
        <a name="rawscl">Tuning Raw File</a>:
           )HTML"
          << synth->storage.currentScale.name << "</div><br/>\n<pre>\n"
          << synth->storage.currentScale.rawText << R"HTML(
      </pre>
    </div>
)HTML";

    if (!synth->storage.isStandardMapping)
    {
        htmls << R"HTML(
    <div style="margin:10pt; padding: 5pt; border: 1px solid #123463; background: #fafbff;">
      <div style="font-size: 13pt; font-family: Lato; font-weight: 600; color: #123463;">
        <a name="rawkbm">Keyboard Mapping Raw File</a>:
           )HTML"
              << synth->storage.currentMapping.name << "</div><br/>\n<pre>\n"
              << synth->storage.currentMapping.rawText << R"HTML(
      </pre>
    </div>
)HTML";
    }

    // Interval matrices
    htmls << R"HTML(
    <div style="margin:10pt; padding: 5pt; border: 1px solid #123463; background: #fafbff;">
        <div style="font-size: 13pt; font-family: Lato; font-weight: 600; color: #123463;">
        <a name="matrices">Interval Matrices</a>:
           )HTML"
          << synth->storage.currentScale.name << "</div><br/>\n";

    if (synth->storage.currentMapping.count > 48)
    {
        htmls << "Surge XT only displays interval matrices for scales lower than 48 in length"
              << std::endl;
    }
    else
    {
        int w = synth->storage.currentScale.count;
        htmls << "<table><tr>";
        for (int i = 0; i <= w; ++i)
        {
            htmls << "<th>" << i << "</th>";
        }
        htmls << "</tr></td>";

        // Do a trick of rotating by double filling cents so we don't have to index wrap
        std::vector<float> cents;
        float lastc = 0;
        cents.push_back(0);
        for (auto &t : synth->storage.currentScale.tones)
        {
            cents.push_back(t.cents);
            lastc = t.cents;
        }
        for (auto &t : synth->storage.currentScale.tones)
        {
            cents.push_back(t.cents + lastc);
        }

        htmls << "<tr><th colspan=\"" << w + 1 << "\">Degrees under Rotation</th></tr>";

        for (int rd = 0; rd < w; ++rd)
        {
            htmls << "<tr class=\"cnt\"><th>" << rd << "</th>";

            for (int p = 1; p <= w; ++p)
            {
                htmls << "<td>" << std::setw(8) << std::setprecision(1) << std::fixed
                      << cents[p + rd] - cents[rd] << "</td>";
            }

            htmls << "</tr>";
        }

        htmls << "<tr><th colspan=\"" << w + 1 << "\">Intervals under Rotation</th></tr>";

        for (int rd = 0; rd < w; ++rd)
        {
            htmls << "<tr class=\"cnt\"><th>" << rd << "</th>";

            for (int p = 1; p <= w; ++p)
            {
                htmls << "<td>" << std::setw(8) << std::setprecision(1) << std::fixed
                      << cents[p + rd] - cents[p + rd - 1] << "</td>";
            }

            htmls << "</tr>";
        }

        htmls << "</table>";
    }

    htmls << R"HTML(
    </div>
)HTML";

    htmls << R"HTML(
  </body>
</html>
      )HTML";

    return htmls.str();
}

std::string SurgeGUIEditor::midiMappingToHtml()
{
    std::ostringstream htmls;

    htmls <<
        R"HTML(
<html>
  <head>
    <link rel="stylesheet" type="text/css" href="https://fonts.googleapis.com/css?family=Lato" />
    <style>
table {
  border-collapse: collapse;
}

td {
  border: 1px solid #CDCED4;
  padding: 2pt 4px;
}

.center {
  text-align: center;
}

th {
  padding: 4pt;
  color: #123463;
  background: #CDCED4;
  border: 1px solid #123463;
}
</style>
  </head>
  <body style="margin: 0pt; background: #CDCED4;">
    <div style="border-bottom: 1px solid #123463; background: #ff9000; padding: 2pt;">
      <div style="font-size: 20pt; font-family: Lato; padding: 2pt; color:#123463;">
        Surge XT MIDI Mapping
      </div>
    </div>

    <div style="margin:10pt; padding: 5pt; border: 1px solid #123463; background: #fafbff;">
      <div style="font-size: 12pt; margin-bottom: 10pt; font-family: Lato; color: #123463;">

     )HTML";
    // TODO: if there are none print differently
    bool foundOne = false;
    int n = n_global_params + n_scene_params;
    for (int i = 0; i < n; i++)
    {
        if (synth->storage.getPatch().param_ptr[i]->midictrl >= 0)
        {
            if (!foundOne)
            {
                foundOne = true;
                htmls << "Individual parameter MIDI mappings<p>\n"
                      << "<table><tr><th>CC#</th><th>Parameter</th></tr>\n";
            }
            htmls << "<tr><td class=\"center\">" << synth->storage.getPatch().param_ptr[i]->midictrl
                  << "</td><td> " << synth->storage.getPatch().param_ptr[i]->get_full_name()
                  << "</td></tr>\n";
        }
    }
    if (foundOne)
    {
        htmls << "</table>\n";
    }
    else
    {
        htmls << "No parameter MIDI mappings present!";
    }

    htmls << R"HTML(

      </div>
    </div>
    <div style="margin:10pt; padding: 5pt; border: 1px solid #123463; background: #fafbff;">
      <div style="font-size: 12pt; margin-bottom: 10pt; font-family: Lato; color: #123463;">
         Macro Assignments<p>
         <table><tr><th>CC#</th><th>Macro</th><th>Custom Name</th></tr>
     )HTML";
    for (int i = 0; i < n_customcontrollers; ++i)
    {
        std::string name = synth->storage.getPatch().CustomControllerLabel[i];
        auto ccval = synth->storage.controllers[i];

        htmls << "<tr><td class=\"center\">" << (ccval == -1 ? "N/A" : std::to_string(ccval))
              << "</td><td class=\"center\">" << i + 1 << "</td><td>" << name << "</td></tr>"
              << std::endl;
    }
    htmls << R"HTML(
         </table>
      </div>
    </div>
  </body>
</html>
      )HTML";

    return htmls.str();
}

// for sorting parameters for display:
struct oscParamInfo
{
    Parameter *p;
    std::string storage_name;
    std::string full_name;
    int ctrlgroup;
};

// Sort function for displaying parameters (below)
bool compareParams(const oscParamInfo &opl, const oscParamInfo &opr)
{
    int cgroup_order[10] = {0, 1, 2, 4, 3, 5, 6, 7, 8, 9};

    int lcg = cgroup_order[opl.p->ctrlgroup];
    int rcg = cgroup_order[opr.p->ctrlgroup];
    if (lcg == rcg)
    {
        std::string ls = opl.storage_name;
        std::string rs = opr.storage_name;
        return strnatcasecmp(ls.c_str(), rs.c_str()) < 0;
    }
    return lcg < rcg;
};

std::string SurgeGUIEditor::parametersToHtml()
{
    std::ostringstream htmls;

    htmls <<
        R"HTML(
<html>
  <head>
    <link rel="stylesheet" type="text/css" href="https://fonts.googleapis.com/css?family=Lato" />
    <style>
table {
  border-collapse: collapse;
  width: 100%;

}

td {
  border: 1px solid #CDCED4;
  padding: 2pt 8px;
}

.center {
  text-align: center;
}

div.heading {
    margin: 4px 0px 0px 8px;
    width: 100%;
}

h3 {
    margin: 0 0 8px 0;
}

div.outer {
    font-size: 12pt;
    margin: 0 0 10pt 0;
    font-family: Lato;
    color: #123463;
}

div.frame {
    max-width: 1300px;
}

div.tablewrap {
    width: 610px;
    margin: 0 8px 16px 8px;
    box-sizing: border-box;
    display: block;
    border: 1px solid #ccc;
    padding: 10px;
    margin-top: 10px;
}

th {
  padding: 4pt;
  color: #123463;
  background: #CDCED4;
  border: 1px solid #123463;
}

span {
    margin: 2px; 20px;
    padding: 1px 2px;
    border: 1px solid #123463;
    white-space:nowrap;
}

ul {
    margin: 10px 0;
}

.cl{
   clear: left;
}
.cr{
   clear: right;
}
.fl{
   float: left;
}
.fr{
   float: right;
}
* {
   -webkit-box-sizing: border-box;
   -moz-box-sizing: border-box;
}


code {
    color: #dc3918;
    font-size: 16px;
    padding: 1px 0px;
}
</style>
  </head>
  <body style="margin: 0pt; background: #CDCED4;">
    <div style="border-bottom: 1px solid #123463; background: #ff9000; padding: 2pt;">
      <div style="font-size: 20pt; font-family: Lato; padding: 2pt; color:#123463;">
        Surge XT Open Sound Control (OSC) Specification
      </div>
    </div>

    <div style="margin:10pt; padding: 5pt; border: 1px solid #123463; background: #fafbff;">
      <div style="line-height: 1.5; font-size: 12pt; font-family: Lato; color: #123463;">
        Construct OSC messages using the exact (case sensitive)
        entry listed in the <b>Address</b> column in the tables below.</br>
        The form of the message should be <code>/&ltaddress&gt &ltvalue&gt</code>,
        where <code>address</code> can currently begin with either <code>tuning</code>, <code>settings</code>, or <code>param</code>,</br> and <code>value</code> can be:

        <ul>
            <li>a floating point value between <b>0.0</b> and <b>1.0</b></li>
            <li>an integer value</li>
            <li>a boolean value, <b>0</b> (false) or <b>1</b> (true)</li>
            <li>a file path (absolute, or relative to the default path)
            <li>contextual: either an in integer or a float, depending on the context (loaded oscillator or effect type)</li>
        </ul>

        Where an address contains an asterisk <b>(*)</b>, replace the asterisk with either <b>a</b> or <b>b</b>,
        depending on which scene you wish to address - e.g. <code>/a/drift</code> or <code>/b/drift</code>.

        <p>Examples:
            <div style="margin: -6px 0 2px 0; line-height: 1.75">
                <span><code>/param/b/amp/gain 0.63</code></span>
                <span><code>/param/global/polyphony_limit 12</code></span>
                <span><code>/param/a/mixer/noise/mute 0</code></span>
            </div>
            <div style="margin: 4px 0 0 0; line-height: 1.75">
                <span><code>/tuning/scl ptolemy</code></span>
                <span><code>/tuning/scl /Users/jane/scala_tunings/ptolemy.scl</code></span>
                <span><code>/settings/path/scl /Users/jane/scala_tunings</code></span>
            </div>
        </p>
      </div>
    </div>

    <div style="margin:10pt; padding: 5pt; border: 1px solid #123463; background: #fafbff; overflow:hidden">
      <div class="outer"><div class="frame">
    )HTML";

    // Show tuning controls
    htmls << R"HTML(
        <div class="tablewrap fl cl">
        <div class="heading"><h3>Tuning:</h3></div>
            <table style="border: 2px solid black;">
                <tr>
                    <th>Address</th>
                    <th>Description</th>
                    <th>Appropriate Values</th>
                </tr>
                <tr>
                    <td>/tuning/scl</td>
                    <td> .scl tuning file</td>
                    <td class="center">file path (absolute or relative)</td>
                </tr>
                <tr>
                    <td>/tuning/kbm</td>
                    <td>.kbm mapping file</td>
                    <td class="center">file path (absolute or relative)</td>
                </tr>
            </table>
            </div>
        )HTML";

    // Show settings
    htmls << R"HTML(
        <div class="tablewrap fr cr">
        <div class="heading"><h3>Settings:</h3></div>
            <table style="border: 2px solid black;">
                <tr>
                    <th>Address</th>
                    <th>Description</th>
                    <th>Appropriate Values</th>
                </tr>
                <tr>
                    <td>/settings/path/scl</td>
                    <td>.scl file default path</td>
                    <td class="center">file path (absolute only)</td>
                </tr>
                <tr>
                    <td>/settings/path/kbm</td>
                    <td>.kbm file default path</td>
                    <td class="center">file path (absolute only)</td>
                </tr>
                <tr>
                    <td class="center" colspan="3">(use value = '_reset' to reset path to factory default)</td>
                </tr>
            </table>
            </div>
        )HTML";

    std::vector<oscParamInfo> sortvector;
    std::string st_str;
    int currentCtrlGrp = endCG;

    for (auto *p : synth->storage.getPatch().param_ptr)
    {
        st_str = p->get_osc_name();

        if (st_str[6] == '/' && st_str[8] == '/')
        {
            if (st_str[7] == 'b')
                continue; // 'b_...' entries not added to vector

            else if (st_str[7] == 'a')
            {
                st_str[7] = '*';
            }
        }
        sortvector.push_back(oscParamInfo{p, st_str, p->get_full_name(), p->ctrlgroup});
    };

    // Sort by control group number, storage name (natural sort)
    std::sort(sortvector.begin(), sortvector.end(), compareParams);

    // Generate HTML table of parameters
    int tabnum = 0;
    for (auto itr : sortvector)
    {
        bool skip = false;
        std::string valueType;

        if (itr.p->ctrlgroup != currentCtrlGrp)
        {
            if (currentCtrlGrp != endCG)
            {
                htmls << "</table></div>";
            }
            currentCtrlGrp = itr.p->ctrlgroup;
            if (tabnum++ % 2 == 0) // Even number?
            {
                htmls << "<div class=\"tablewrap fl cl\">";
            }
            else
            {
                htmls << "<div class=\"tablewrap fr cr\">";
            }

            htmls << "<div class=\"heading\"><h3>Control Group: "
                  << ControlGroupDisplay[itr.p->ctrlgroup] << "</h3></div>"
                  << R"HTML(
                
            <table style="border: 2px solid black;">
                <tr>
                    <th>Address</th>
                    <th>Parameter Name</th>
                    <th>Appropriate Values</th>
                </tr>
        )HTML";
        }

        if (itr.ctrlgroup == cg_OSC || itr.ctrlgroup == cg_FX)
        {
            valueType = "(contextual)";
        }
        else if (itr.p->ctrltype != ct_none)
        {
            switch (itr.p->valtype)
            {
            case vt_int:
                valueType = "integer (" + std::to_string(itr.p->val_min.i) + " to " +
                            std::to_string(itr.p->val_max.i) + ")";
                break;

            case vt_bool:
                valueType = "boolean (0 or 1)";
                break;

            case vt_float:
            {
                valueType = "float (0.0 to 1.0)";
                break;
            }

            default:
                break;
            }
        }
        else
            skip = true;

        if (!skip)
        {
            htmls << "<tr><td>" << itr.storage_name << "</td><td> " << itr.p->get_full_name()
                  << "</td>"
                  << "<td class=\"center\">" << valueType << "</td></tr>";
        }
    }

    htmls << R"HTML(
                        </table>
                    </div>
                </div>
            </div>
        </body>
        </html>
      )HTML";
    return htmls.str();
}

std::string SurgeGUIEditor::skinInspectorHtml(SkinInspectorFlags f)
{
    std::ostringstream htmls;

    auto startSection = [&htmls](std::string secname, std::string seclabel) {
        htmls << R"HTML(
     <div style="margin:10pt; padding: 5pt; border: 1px solid #123463; background: #fafbff;">
                <div style="font-size: 12pt; margin-bottom: 10pt; font-family: Lato; color: #123463;">

          )HTML"
              << "<a name=\"" << secname << "\"><h2>" << seclabel << "</h2></a>\n";
    };
    auto endSection = [&htmls]() {
        htmls << R"HTML(
        </div></div>
          )HTML";
    };
    htmls << R"HTML(
   <html>
   <head>
   <link rel="stylesheet" type="text/css" href="https://fonts.googleapis.com/css?family=Lato" />
                                               <style>
                                               table {
                                                   border-collapse: collapse;
                                               }

   td {
       border: 1px solid #CDCED4;
       padding: 2pt 4px;
   }

       .center {
      text-align: center;
   }

   th {
       padding: 4pt;
       color: #123463;
       background: #CDCED4;
       border: 1px solid #123463;
   }
   </style>
     </head>
       <body style="margin: 0pt; background: #CDCED4;">
                   <div style="border-bottom: 1px solid #123463; background: #ff9000; padding: 2pt;">
                              <div style="font-size: 20pt; font-family: Lato; padding: 2pt; color:#123463;">
                                         Surge XT Skin Inspector
                   </div>
                     </div>


    <div style="margin:10pt; padding: 5pt; border: 1px solid #123463; background: #fafbff;">
      <div style="font-size: 12pt; margin-bottom: 10pt; font-family: Lato; color: #123463;">
        <ul>
          <li><a href="#colors">Color Database</a>
          <li><a href="#imageid">Image IDs</a>
          <li><a href="#classes">Built-in Classes and their Properties</a>
          <li><a href="#skinConnectors">Skin Component Connectors</a>
          <li><a href="#loadedImages">Runtime Loaded Images</a>
        </ul>
      </div>
    </div>

   )HTML";

    startSection("colors", "Color Database");
    htmls << "<table><tr><th>Color Name</th><th colspan=2>Default Color</th><th colspan=2>Current "
             "Color</th></tr>\n";
    auto cols = Surge::Skin::Color::getAllColors();

    auto htmlBlob = [](int r, int g, int b, int a) {
        std::ostringstream rs;
        // rs << "rgba(" << r << "," << g << "," << b << "," << a << ") / ";
        rs << "#" << std::hex << std::setw(2) << std::setfill('0') << r << std::setw(2)
           << std::setfill('0') << g << std::setw(2) << std::setfill('0') << b;
        if (a != 255)
            rs << std::setw(2) << std::setfill('0') << a;
        auto colh = rs.str();

        std::ostringstream cells;
        cells << "<td>" << colh << "</td><td width=10 style=\"background-color: " << colh
              << "\">&nbsp;</td>";
        return cells.str();
    };

    for (auto &c : cols)
    {
        auto skincol = currentSkin->getColor(c);
        htmls << "<tr><td>" << c.name << "</td>" << htmlBlob(c.r, c.g, c.b, c.a)
              << htmlBlob(skincol.getRed(), skincol.getGreen(), skincol.getBlue(),
                          skincol.getAlpha())
              << "</tr>\n";
    }
    htmls << "</table>";
    endSection();

    startSection("imageid", "Image IDs");

    {
        auto q = Surge::GUI::createIdNameMap();

        std::vector<std::pair<std::string, int>> asV(q.begin(), q.end());
        std::sort(asV.begin(), asV.end(), [](auto a, auto b) { return a.second < b.second; });
        htmls << "<table><tr><th>Resource Number</th><th>Resource ID</th></tr>\n";
        for (auto v : asV)
        {
            htmls << "<tr><td>" << v.second << "</td><td>" << v.first << "</td></tr>\n";
        }
        htmls << "</table>\n";
    }
    endSection();

    htmls << R"HTML(
  </body>
</html>
      )HTML";

    // CLasses
    startSection("classes", "Built-in classes and their properties ");
    {
        auto clid = Surge::Skin::Component::allComponentIds();
        for (auto cl : clid)
        {
            auto comp = Surge::Skin::Component::componentById(cl);
            htmls << "<p><b>" << comp.payload->internalClassname << " / " << comp.payload->id
                  << "</b></p>\n";
            htmls << "<table><tr><th>Property</th><th>Skin Property XML</th><th>Doc</th></tr>";
            std::set<Surge::Skin::Component::Properties> cheapSort;
            for (auto const &pnp : comp.payload->propertyNamesMap)
            {
                cheapSort.insert(pnp.first);
            }
            for (auto id : cheapSort)
            {
                auto doc = comp.payload->propertyDocString[id];
                auto vec = comp.payload->propertyNamesMap[id];
                auto prp = Surge::Skin::Component::propertyEnumToString(id);

                htmls << "<tr><td>" << prp << "</td><td>";
                std::string pre = "";
                for (auto const &sv : vec)
                {
                    htmls << pre << sv << " ";
                    pre = "/ ";
                }
                htmls << "</td><td>" << doc << "</td></tr>";
            }
            htmls << "</table>";
        }
    }
    endSection();

    // Skin Connectors
    startSection("skinConnectors", "Skin Component Connectors");
    {
        auto imgid = Surge::GUI::createIdNameMap();

        auto co = Surge::Skin::Connector::allConnectorIDs();
        htmls << "<table><tr><th>Name</th><th>Geometry "
                 "(XxY+WxH)</th><th>Class</th><th>Properties</th></tr>\n";
        for (auto c : co)
        {
            htmls << "<tr><td>" << c << "</td><td>";
            auto comp = currentSkin->getOrCreateControlForConnector(
                Surge::Skin::Connector::connectorByID(c));
            if (comp)
            {
                htmls << comp->x << "x" << comp->y;
                if (comp->w > 0 || comp->h > 0)
                    htmls << "+" << comp->w << "x" << comp->h;
            }

            htmls << "</td><td>";
            if (comp)
            {
                if (comp->classname == comp->ultimateparentclassname)
                    htmls << comp->classname;
                else
                    htmls << comp->classname << " : " << comp->ultimateparentclassname;
            }

            htmls << "</td><td>";
            if (comp)
                for (auto p : comp->allprops)
                {
                    if (p.first != "x" && p.first != "y" && p.first != "w" && p.first != "h")
                    {
                        htmls << p.first << ": " << p.second;
                        if (p.first == "bg_id")
                        {
                            int id = std::atoi(p.second.c_str());
                            // Linear search kinda sucks but this is debugging code for UI
                            // generation on small list
                            for (auto q : imgid)
                                if (q.second == id)
                                    htmls << " (" << q.first << ")\n";
                        }
                        htmls << "<br>\n";
                    }
                }
            htmls << "</td></tr>\n";
        }
        htmls << "</table>\n";
    }
    endSection();

    // Loaded Images
    startSection("loadedImages", "Runtime Loaded Images");
    {
        htmls << "<ul>\n";
        auto r1 = bitmapStore->nonResourceBitmapIDs(SurgeImageStore::STRINGID);
        for (auto v : r1)
        {
            htmls << "<li>" << v << "</li>\n";
        }
        auto r2 = bitmapStore->nonResourceBitmapIDs(SurgeImageStore::PATH);
        for (auto v : r2)
        {
            htmls << "<li>" << v << "</li>\n";
        }
        htmls << "</ul>\n";
    }
    endSection();
    return htmls.str();
}

std::string SurgeGUIEditor::patchToHtml(bool includeDefaults)
{
    std::ostringstream htmls;
    std::string tab2 = "\t\t";
    std::string tab3 = tab2 + "\t";
    std::string tab4 = tab3 + "\t";
    std::string tab5 = tab4 + "\t";

    auto dumpParam = [&htmls, includeDefaults, tab3](const Parameter &p) {
        if (p.ctrltype == ct_none)
        {
            return;
        }

        if (includeDefaults || p.get_value_f01() != p.get_default_value_f01())
        {
            htmls << tab3 << "<b>" << p.get_name() << ":</b> " << p.get_display() << "<br/>\n";
        }
    };

    const auto &patch = synth->storage.getPatch();

    std::vector<int> scenes;

    if (patch.scenemode.val.i == sm_single)
    {
        scenes.push_back(patch.scene_active.val.i);
    }
    else
    {
        for (int i = 0; i < n_scenes; ++i)
        {
            scenes.push_back(i);
        }
    }

    htmls << R"HTML(<html>
    <head>
        <link rel ="stylesheet" type="text/css" href="https://fonts.googleapis.com/css?family=Lato"/>

        <style>
            h1, h2, h3 { font-family: Lato; color: #123463; margin: 6pt; margin-left: 12pt; }

            h1 { font-size: 20pt; }
            h2 { font-size: 16pt; }
            h3 { font-size: 13pt; margin-left: 8pt; }

            p { font-family: Lato; font-size: 12pt; margin: 2pt; line-height: 1.5; padding-left: 4pt; }

            .header { border-bottom: 1px solid #123463; background: #ff9000; padding: 2pt; }

            .frame { margin:10pt; padding: 5pt; border: 1px solid #123463; background: #fafbff; }
        </style>
    </head>

    <body style="font-family: Lato; margin: 0pt; background: #CDCED4">

        <div class="header">
            <div style="font-size: 20pt; padding: 2pt; color: #123463;">
                Surge XT Patch Export To Text
            </div>
        </div>
)HTML";

    auto cat = patch.category;
    std::replace(cat.begin(), cat.end(), '\\', '/');

    htmls << std::endl << tab2 << "<div class=\"frame\">\n";
    htmls << tab3 << "<p>" << std::endl;
    htmls << tab4 << "<b>Patch Name:</b> " << patch.name << "<br/> " << std::endl
          << tab4 << "<b>Category:</b> " << cat << std::endl
          << tab3 << "</p>" << std::endl;
    htmls << tab2 << "</div>\n";

    // global stuff - poly limit, fx bypass, character
    htmls << std::endl << tab2 << "<div class=\"frame\">\n";
    htmls << tab3 << "<h2>Global Patch Settings</h2>\n" << tab3 << "<p>\n";

    for (const auto &par : {patch.scene_active, patch.scenemode, patch.splitpoint, patch.volume,
                            patch.polylimit, patch.fx_bypass, patch.character})
    {
        htmls << "\t";
        dumpParam(par);
    }

    htmls << tab3 << "</p>\n";
    htmls << tab2 << "</div>\n";

    std::ostringstream mods;
    std::ostringstream globmods;
    std::set<std::pair<int, int>> modsourceSceneActive;

    for (const auto &mr : patch.modulation_global)
    {
        char depth[TXT_SIZE];
        auto p = patch.param_ptr[mr.destination_id];
        int ptag = p->id;
        auto thisms = (modsources)mr.source_id;

        p->get_display_of_modulation_depth(
            depth, synth->getModDepth(ptag, thisms, mr.source_scene, mr.source_index),
            synth->isBipolarModulation(thisms), Parameter::Menu);

        globmods << tab4 << "<b>" << modulatorName(mr.source_id, false) << " -> "
                 << p->get_full_name() << ":</b> " << depth << "<br/>\n";
        modsourceSceneActive.emplace(mr.source_id, mr.source_scene);
    }

    if (globmods.tellp() != std::streampos(0))
    {
        mods << std::endl << tab2 << "<div class=\"frame\">\n";
        mods << tab3 << "<h2>Global Modulation Targets</h2>\n" << tab3 << "<p>" << std::endl;
        mods << globmods.str();
        mods << tab3 << "</p>\n";
        mods << tab2 << "</div>\n";
    }

    for (const auto &scn : scenes)
    {
        std::ostringstream scenemods;
        int idx{0};

        for (const auto &[rt, n] : {std::make_pair(patch.scene[scn].modulation_voice, "Voice"),
                                    {patch.scene[scn].modulation_scene, "Scene"}})
        {
            std::ostringstream curscenemods;

            for (const auto &mr : rt)
            {
                char depth[TXT_SIZE];
                auto p = patch.param_ptr[mr.destination_id + patch.scene_start[scn]];
                int ptag = p->id;
                auto thisms = (modsources)mr.source_id;

                p->get_display_of_modulation_depth(
                    depth, synth->getModDepth(ptag, thisms, scn, mr.source_index),
                    synth->isBipolarModulation(thisms), Parameter::Menu);

                curscenemods << tab4 << "<b>" << modulatorName(thisms, false) << " -> "
                             << p->get_full_name() << ":</b> " << depth << "<br/>\n";
                modsourceSceneActive.emplace(thisms, mr.source_scene);
            }

            if (curscenemods.tellp() != std::streampos(0))
            {
                scenemods << tab3 << "<h2>Scene " << (char)('A' + scn) << " " << n
                          << " Modulation Targets</h2>\n"
                          << tab3 << "<p>\n";
                scenemods << curscenemods.str();
                scenemods << tab3 << "</p>\n";

                if (idx == 0)
                {
                    scenemods << std::endl;
                }
            }

            idx++;
        }

        if (scenemods.tellp() != std::streampos(0))
        {
            mods << std::endl << tab2 << "<div class=\"frame\">\n";
            mods << scenemods.str();
            mods << tab2 << "</div>\n";
        }
    }

    for (const auto &scn : scenes)
    {
        htmls << std::endl << tab2 << "<div class=\"frame\">" << std::endl;
        htmls << tab3 << "<h2>Scene " << (char)('A' + scn) << "</h2>" << std::endl;

        const auto &sc = patch.scene[scn];
        int idx{1};

        htmls << std::endl
              << tab3 << "<h3>Scene Settings</h3>" << std::endl
              << tab3 << "<p>" << std::endl;

        for (const auto &par : {sc.polymode, sc.portamento, sc.pitch, sc.octave, sc.pbrange_dn,
                                sc.pbrange_dn, sc.keytrack_root, sc.drift})
        {
            htmls << "\t";
            dumpParam(par);
        }

        htmls << tab3 << "</p>" << std::endl;

        for (const auto &o : sc.osc)
        {
            htmls << std::endl
                  << tab3 << "<h3>Oscillator " << idx << ": " << o.type.get_display() << "</h3>"
                  << std::endl;
            htmls << tab3 << "<p>\n";

            const auto *p = &(o.type);
            const auto *e = &(o.retrigger);

            p++;

            while (p <= e)
            {
                htmls << "\t";
                dumpParam(*p);
                p++;
            }

            if (uses_wavetabledata(o.type.val.i))
            {
                htmls << tab4 << "<b>Wavetable:</b> " << o.wavetable_display_name << std::endl;
            }

            htmls << tab3 << "</p>\n";

            idx++;
        }

        htmls << std::endl
              << tab3 << "<h3>Oscillator FM</h3>" << std::endl
              << tab3 << "<p>" << std::endl;

        for (const auto &par : {sc.fm_switch, sc.fm_depth})
        {
            htmls << "\t";
            dumpParam(par);
        }

        htmls << tab3 << "</p>" << std::endl;

        htmls << std::endl << tab3 << "<h3>Mixer</h3>\n" << tab3 << "<p>" << std::endl;

        {
            const auto *p = &(sc.level_o1);
            const auto *e = &(sc.route_ring_23);

            while (p <= e)
            {
                htmls << "\t";
                dumpParam(*p);
                p++;
            }

            htmls << tab3 << "</p>" << std::endl;
        }

        if (sc.wsunit.type.val.i != (int)sst::waveshapers::WaveshaperType::wst_none)
        {
            htmls << std::endl
                  << tab3 << "<h3>Waveshaper</h3>" << std::endl
                  << tab3 << "<p>" << std::endl;

            htmls << "\t";
            dumpParam(sc.wsunit.type);
            htmls << "\t";
            dumpParam(sc.wsunit.drive);

            htmls << tab3 << "</p>" << std::endl;
        }

        idx = 1;

        for (const auto &f : sc.filterunit)
        {
            if (f.type.val.i != sst::filters::fut_none)
            {
                htmls << std::endl
                      << tab3 << "<h3>Filter " << idx << "</h3>" << std::endl
                      << tab3 << "<p>" << std::endl;

                htmls << tab4 << "<b>Type:</b> " << f.type.get_display() << "<br/>" << std::endl;
                htmls << tab4 << "<b>Subtype:</b> " << f.subtype.get_display() << "<br/>"
                      << std::endl;

                const auto *p = &(f.cutoff);
                const auto *e = &(f.keytrack);

                while (p <= e)
                {
                    htmls << "\t";
                    dumpParam(*p);
                    p++;
                }

                if (idx == 2)
                {
                    htmls << "\t";
                    dumpParam(sc.f2_cutoff_is_offset);
                    htmls << "\t";
                    dumpParam(sc.f2_link_resonance);
                }

                htmls << tab3 << "</p>" << std::endl;

                idx++;
            }
        }

        htmls << std::endl
              << tab3 << "<h3>Filter Block</h3>" << std::endl
              << tab3 << "<p>" << std::endl;

        for (const auto &par :
             {sc.filterblock_configuration, sc.filter_balance, sc.feedback, sc.lowcut})
        {
            htmls << "\t";
            dumpParam(par);
        }

        htmls << tab3 << "</p>" << std::endl;

        htmls << std::endl
              << tab3 << "<h3>Amplifier</h3>" << std::endl
              << tab3 << "<p>" << std::endl;

        for (const auto &par : {sc.vca_level, sc.vca_velsense})
        {
            htmls << "\t";
            dumpParam(par);
        }

        htmls << tab3 << "</p>" << std::endl << std::endl;

        htmls << tab3 << "<h3>Scene Output</h3>" << std::endl << tab3 << "<p>" << std::endl;

        {
            const auto *p = &(sc.volume);
            const auto *e = &(sc.send_level[n_send_slots - 1]);

            while (p <= e)
            {
                htmls << "\t";
                dumpParam(*p);
                p++;
            }

            htmls << tab3 << "</p>" << std::endl;
        }

        for (const auto &idx : {ms_ampeg, ms_filtereg})
        {
            const auto &en = sc.adsr[idx - ms_ampeg];

            htmls << std::endl
                  << tab3 << "<h3>" << (idx == ms_ampeg ? "Amplifier EG" : "Filter EG") << "</h3>"
                  << std::endl
                  << tab3 << "<p>" << std::endl;

            const auto *p = &(en.a);
            const auto *e = &(en.mode);

            while (p <= e)
            {
                htmls << "\t";
                dumpParam(*p);
                p++;
            }

            htmls << tab3 << "</p>" << std::endl;
        }

        for (const auto &[li, lf] : sst::cpputils::enumerate(sc.lfo))
        {
            auto ms = ms_lfo1 + li;

            if (modsourceSceneActive.find({ms, scn}) != modsourceSceneActive.end())
            {
                htmls << std::endl
                      << tab3 << "<h3>" << modulatorName(ms, false) << "</h3>" << std::endl
                      << tab3 << "<p>" << std::endl;

                const auto *p = &(lf.rate);
                const auto *e = &(lf.release);

                while (p <= e)
                {
                    htmls << "\t";
                    dumpParam(*p);
                    p++;
                }

                htmls << tab3 << "</p>" << std::endl;
            }
        }

        htmls << tab2 << "</div>" << std::endl;
        // Scenes
    }

    // FX
    htmls << std::endl << tab2 << "<div class=\"frame\">" << std::endl;
    htmls << tab3 << "<h2>Effects</h2>" << std::endl;

    for (int i = 0; i < n_fx_slots; ++i)
    {
        if (includeDefaults || patch.fx[fxslot_order[i]].type.val.i != fxt_off)
        {
            const auto &fx = patch.fx[fxslot_order[i]];

            htmls << std::endl
                  << tab3 << "<h3>" << fxslot_names[fxslot_order[i]] << ": "
                  << fx_type_names[fx.type.val.i] << "</h3>\n"
                  << tab3 << "<p>\n";

            if (fxslot_order[i] == fxslot_send1 || fxslot_order[i] == fxslot_send2 ||
                fxslot_order[i] == fxslot_send3 || fxslot_order[i] == fxslot_send4)
            {
                htmls << "\t";
                dumpParam(fx.return_level);
            }

            for (const auto &p : fx.p)
            {
                if (p.ctrltype != ct_none)
                    htmls << "\t";

                dumpParam(p);
            }

            htmls << tab3 << "</p>\n";
        }
    }

    htmls << tab2 << "</div>" << std::endl << mods.str();

    htmls << "\t</body>\n</html>";

    return htmls.str();
}
-- Javascript embedding plugin for Gitit.
--
-- The plugin converts a code block like this:
--
-- ~~~{.javascript src="uri"}
-- whatever
-- ~~~
--
-- into a <script> tag referring to an external resource
--
-- or a code block like this:
--
-- ~~~{.javascript}
-- javascript code
-- ~~~
--
-- into an embedded script verbatim.

module JavaScript (plugin) where

import Network.Gitit.Interface
import Network.URI
import Text.XHtml

plugin :: Plugin
plugin = mkPageTransformM transformBlock

transformBlock (CodeBlock (_, classes, namevals) contents) | "javascript" `elem` classes = 
  case lookup "src" namevals of
    Just uri -> extjs uri
    Nothing  -> embjs contents

transformBlock x = return x

extjs uri = do
  let tagscr = script ! [src uri, thetype "text/javascript"] << noHtml
  return $ RawHtml $ showHtmlFragment tagscr

embjs cnts = do
  let tagscr = script ! [thetype "text/javascript"] << primHtml cnts
  return $ RawHtml $ showHtmlFragment tagscr



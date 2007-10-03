<?cs include:"templates/header.cs" ?>
<?cs each:ikesa = ikesas ?>
  <div class="expand" id="ikesa-<?cs name:ikesa ?>">
  <h1>
  	IKE #<?cs name:ikesa ?> [<?cs var:ikesa.peerconfig ?>]:
  	<span><?cs var:ikesa.local.identification ?></span> &lt;-&gt; 
  	<span><?cs var:ikesa.remote.identification ?></span>
  </h1> 
  <div>
    <hr/>
    <table class="drawing">
      <tr>
        <td class="left" colspan="3">
          <?cs var:ikesa.local.identification ?>
        </td>
        <td>
        </td>
        <td class="right" colspan="3">
          <?cs var:ikesa.remote.identification ?>
        </td>
      </tr>
      <tr class="images">
        <td>
          <?cs if:ikesa.role == "initiator" ?>
          <img title="Local host is the initiator" src="<?cs var:base ?>/static/client-left.png"></img>
          <?cs else ?>
          <img title="Local host is the responder" src="<?cs var:base ?>/static/gateway-left.png"></img>
          <?cs /if ?>
        </td>
        <td style="background-image:url(<?cs var:base ?>/static/pipe.png)">
	      <?cs var:ikesa.local.spi ?><br/><br/><br/> 
	      <?cs var:ikesa.local.address ?>
        </td>
        <td>
          <?cs if:ikesa.local.nat == "true" ?>
          <img title="Local host is behind NAT" src="<?cs var:base ?>/static/router.png"></img>
          <?cs else ?>
          <img title="Local host is not NATed" src="<?cs var:base ?>/static/pipe.png"></img>
          <?cs /if ?>
        </td>
        <td>
          <?cs if:ikesa.status == "established" ?>
          <img title="IKE connection <?cs var:ikesa.status ?>" src="<?cs var:base ?>/static/pipe-good.png"></img>
          <?cs else ?>
          <img title="IKE connection in state <?cs var:ikesa.status ?>" src="<?cs var:base ?>/static/pipe-bad.png"></img>
          <?cs /if ?>
        </td>
        <td>
          <?cs if:ikesa.remote.nat == "true" ?>
          <img title="Remote host is behind NAT" src="<?cs var:base ?>/static/router.png"></img>
          <?cs else ?>
          <img title="Remote host is not NATed" src="<?cs var:base ?>/static/pipe.png"></img>
          <?cs /if ?>
        </td>
        <td class="right" style="background-image:url(<?cs var:base ?>/static/pipe.png)">
	      <?cs var:ikesa.remote.spi ?><br/><br/><br/> 
	      <?cs var:ikesa.remote.address ?>
        </td>
        <td>
          <?cs if:ikesa.role == "responder" ?>
          <img title="Remote host is the initiator" src="<?cs var:base ?>/static/client-right.png"></img>
          <?cs else ?>
          <img title="Remote host is the responder" src="<?cs var:base ?>/static/gateway-right.png"></img>
          <?cs /if ?>
        </td>
      </tr>
      <?cs each:childsa = ikesa.childsas ?>
      <tr>
      	<td colspan="7" class="expand">
      	  <h1>IPsec #<?cs name:childsa ?> [<?cs var:childsa.childconfig ?>]:</h1>
        </td>
      </tr>
      <tr>
        <td colspan="7"><hr/></td>
      </tr>
      <tr class="images">
      	<td colspan="2">
          <?cs each:net = childsa.local.networks ?>
      	    <p><?cs var:net ?></p>
          <?cs /each ?>
      	</td>
      	<td style="background-image:url(<?cs var:base ?>/static/pipe-thin-left.png)">
          <?cs var:childsa.local.spi ?> &lt;-<br/><br/><br/>
      	</td>
      	<td style="background-image:url(<?cs var:base ?>/static/pipe-thin.png)">
      	</td>
      	<td class="right" style="background-image:url(<?cs var:base ?>/static/pipe-thin-right.png)">
          -&gt; <?cs var:childsa.remote.spi ?><br/><br/><br/>
      	</td>
      	<td class="right" colspan="2">
          <?cs each:net = childsa.remote.networks ?>
      	    <p><?cs var:net ?></p>
          <?cs /each ?>
      	</td>
      </tr>
      <?cs /each ?>
    </table>
  </div>
  </div>
<?cs /each ?>
<?cs include:"templates/footer.cs" ?>
